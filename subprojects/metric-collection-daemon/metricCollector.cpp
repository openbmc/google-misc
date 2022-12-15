#include "metricCollector.hpp"

#include "utils.hpp"

#include <fmt/format.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <syslog.h>
#include <unistd.h>

#include <boost/asio/deadline_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// TODO: Make this configurable
#define TIMER_INTERVAL 10

MetricCollector::MetricCollector(
    boost::asio::io_context& ioc,
    std::shared_ptr<sdbusplus::asio::connection> conn,
    sdbusplus::asio::object_server& objServer) :
    ioc(ioc),
    conn(conn), objServer(objServer)
{
    updateBootCount();
    registerPorts();
    registerDaemons();
    registerAssociations();

    auto metrics = objServer.add_interface(bmcObjectPath, bmcInterfaceName);

    metrics->register_property("BootCount", bootCount);
    metrics->register_property("CrashCount", crashCount);

    metrics->initialize();
}

void MetricCollector::registerPorts()
{
    int numPorts = getNumPorts();

    for (int i = 0; i < numPorts; ++i)
    {
        std::optional<std::string> portId = getPortIdByNum(i);

        if (!portId.has_value())
        {
            continue;
        }

        bmcPorts.emplace(*portId,
                         Port(ioc, conn, objServer, bmcObjectPath, *portId));
    }
}

void MetricCollector::registerAssociations()
{
    addPortAssociations(associations);
    addDaemonAssociations(associations);

    auto association_interface =
        objServer.add_interface(bmcObjectPath, associationInterfaceName);

    association_interface->register_property_r<std::vector<Association>>(
        "Associations", sdbusplus::vtable::property_::emits_change,
        [this](const auto&) { return associations; });
    association_interface->initialize();
}

void MetricCollector::addPortAssociations(
    std::vector<Association>& associations)
{
    for (const auto& [_, port] : bmcPorts)
    {
        associations.emplace_back(
            Association{"port", "bmc", port.getObjectPath()});
    }
}

void MetricCollector::addDaemonAssociations(
    std::vector<Association>& associations)
{
    for (const auto& [_, daemon] : bmcDaemons)
    {
        associations.emplace_back(
            Association{"daemon", "bmc", daemon.getObjectPath()});
    }
}

void MetricCollector::removeAssociation(const std::string objectPath)
{
    for (auto it = associations.begin(); it != associations.end(); ++it)
    {
        auto association = *it;
        if (std::get<2>(association) == objectPath)
        {
            associations.erase(it);
            return;
        }
    }
}

std::optional<std::string>
    MetricCollector::getDaemonObjectPathFromPid(const int pid)
{
    try
    {
        auto m = conn->new_method_call(
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "GetUnitByPID");
        m.append(static_cast<uint32_t>(pid));

        auto reply = conn->call(m);

        sdbusplus::message::object_path objectPathVariant;
        reply.read(objectPathVariant);

        return objectPathVariant.str;
    }
    catch (const std::exception& e)
    {
        return std::nullopt;
    }
}

void MetricCollector::registerDaemons()
{

    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        std::optional<int> pid = isNumericPath(path);

        // Exclude sysinit unit because it is a unit but not a service
        if (!pid.has_value() || *pid == 1)
        {
            continue;
        }

        std::optional<std::string> daemonObjectPath =
            getDaemonObjectPathFromPid(*pid);

        // This is if a process does not have a daemon assocaited with it
        if (!daemonObjectPath.has_value())
        {
            continue;
        }

        std::cerr << "Registering daemon with pid " << *pid << std::endl;

        bmcDaemons.emplace(*pid, Daemon(ioc, conn, objServer, bmcObjectPath,
                                        *daemonObjectPath, *pid));
    }
}

void MetricCollector::updatePorts()
{
    // Check if ports need to be updated
    std::unordered_set<std::string> allPortIds;
    int numPorts = getNumPorts();

    for (int i = 0; i < numPorts; ++i)
    {
        std::optional<std::string> portId = getPortIdByNum(i);

        if (!portId.has_value())
        {
            continue;
        }

        allPortIds.emplace(*portId);
    }

    // Remove nonexistent ports
    for (auto it = bmcPorts.begin(); it != bmcPorts.end(); it++)
    {
        auto portId = it->first;
        auto port = it->second;
        if (!allPortIds.contains(portId))
        {
            bmcPorts.erase(it);
            removeAssociation(port.getObjectPath());
            continue;
        }
        // Update port info of existing ports
        port.updatePortInfo();
    }

    // Add new ports
    for (const auto& portId : allPortIds)
    {
        if (!bmcPorts.contains(portId))
        {
            bmcPorts.emplace(portId,
                             Port(ioc, conn, objServer, bmcObjectPath, portId));
            associations.emplace_back(Association{
                "port", "bmc", bmcPorts.at(portId).getObjectPath()});
        }
    }
}

void MetricCollector::updateDaemons()
{
    std::unordered_set<int> allDaemonPids;

    // Get all current Pids
    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        std::optional<int> pid = isNumericPath(path);

        // Exclude sysinit unit because it is a unit but not a service
        if (!pid.has_value() || *pid == 1)
        {
            continue;
        }

        std::optional<std::string> daemonObjectPath =
            getDaemonObjectPathFromPid(*pid);

        // This is if a process does not have a daemon associated with it
        if (!daemonObjectPath.has_value())
        {
            continue;
        }

        std::cerr << "Found pid " << *pid << std::endl;
        allDaemonPids.emplace(*pid);
    }

    // Remove nonexistent daemons
    for (auto it = bmcDaemons.begin(); it != bmcDaemons.end();)
    {
        auto daemonPid = it->first;
        auto daemon = it->second;
        if (!allDaemonPids.contains(daemonPid))
        {
            std::cerr << "Removing pid " << daemonPid << " "
                      << daemon.getObjectPath() << std::endl;
            it = bmcDaemons.erase(it);
            removeAssociation(daemon.getObjectPath());
            continue;
        }
        std::cerr << "Updating metrics for pid " << daemonPid << std::endl;
        // Update daemon info of existing daemons
        daemon.updateInfo();
        ++it;
    }

    // Add new daemon
    for (const auto& daemonPid : allDaemonPids)
    {
        if (!bmcDaemons.contains(daemonPid))
        {
            std::cerr << "Found new daemon " << daemonPid << std::endl;
            std::optional<std::string> daemonObjectPath =
                getDaemonObjectPathFromPid(daemonPid);

            // This shouldnt happen but sanity check
            if (!daemonObjectPath.has_value())
            {
                continue;
            }

            bmcDaemons.emplace(daemonPid,
                               Daemon(ioc, conn, objServer, bmcObjectPath,
                                      *daemonObjectPath, daemonPid));
            associations.emplace_back(Association{
                "daemon", "bmc", bmcDaemons.at(daemonPid).getObjectPath()});
        }
    }
}

void MetricCollector::update()
{
    std::cerr << "Updating info" << std::endl;
    updatePorts();
    updateDaemons();
}

void MetricCollector::updateBootCount()
{
    conn->async_method_call(
        [this](const boost::system::error_code ec,
               DBusPropertiesMap& properties) {
            if (ec)
            {
                fmt::print(stderr, "Cannot get BMC host properties");
                return;
            }
            std::array<size_t, 3> bootinfo = parseBootInfo();
            bool updated = false;
            for (const auto& it : properties)
            {
                // If last reboot time is different then what is stored,
                // then update info
                if (it.first == "LastRebootTime" &&
                    std::get<uint64_t>(it.second) != bootinfo[2])
                {
                    updated = true;
                    bootinfo[2] = std::get<uint64_t>(it.second);
                }
                if (it.first == "LastRebootCause" && updated)
                {
                    ++bootinfo[0];
                    if (std::get<std::string>(it.second) ==
                        "xyz.openbmc_project.State.BMC.RebootCause.Unknown")
                    {
                        ++bootinfo[1];
                    }
                }
            }

            if (updated)
            {
                std::ofstream bootinfoFile("/var/google/bootinfo");
                bootinfoFile << bootinfo[0] << " " << bootinfo[1] << " "
                             << bootinfo[2] << '\n';
                bootinfoFile.close();
            }

            bootCount = bootinfo[0];
            crashCount = bootinfo[1];
        },
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.State.BMC");
}

int MetricCollector::getNumPorts()
{
    struct ifaddrs* ifaddr;
    int family;
    if (getifaddrs(&ifaddr) == -1)
    {
        return -1;
    }

    int numPorts = 0;

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

        if (family != AF_PACKET || ifa->ifa_data == nullptr)
        {
            continue;
        }

        // Only track AF_PACKETS
        ++numPorts;
    }
    freeifaddrs(ifaddr);
    return numPorts;
}

void metricCollectCallback(std::shared_ptr<boost::asio::deadline_timer> timer,
                           sdbusplus::bus_t& bus, MetricCollector& metricCol)
{
    timer->expires_from_now(boost::posix_time::seconds(TIMER_INTERVAL));
    timer->async_wait(
        [timer, &bus, &metricCol](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                fmt::print(stderr, "sensorRecreateTimer aborted");
                return;
            }
            metricCol.update();
            metricCollectCallback(timer, bus, metricCol);
        });
}

/**
 * @brief Main
 */
int main()
{
    // The io_context is needed for the timer
    boost::asio::io_context io;

    // DBus connection
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    conn->request_name(serviceName.c_str());

    // Add object manager through object_server
    sdbusplus::asio::object_server objectServer(conn);

    auto metricCol = std::make_shared<MetricCollector>(io, conn, objectServer);

    auto metricCollectTimer = std::make_shared<boost::asio::deadline_timer>(io);

    // Start the timer
    io.post([conn, &metricCollectTimer, &metricCol]() {
        metricCollectCallback(metricCollectTimer, *conn, *metricCol);
    });

    io.run();

    return 0;
}
