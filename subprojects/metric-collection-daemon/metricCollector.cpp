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
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>

#include <algorithm>
#include <fstream>
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

namespace
{

MetricCollector::MetricCollector(
    boost::asio::io_context& ioc,
    std::shared_ptr<sdbusplus::asio::connection> conn,
    sdbusplus::asio::object_server& objServer) :
    ioc(ioc),
    conn(conn), objServer(objServer)
{
    registerAssociations();
    updateBootCount();
    registerPorts();
    registerDaemons();

    metrics = objServer.add_interface(bmcObjectPath, bmcInterfaceName);

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

        if (portId == std::nullopt)
        {
            continue;
        }

        bmcPorts.emplace(*portId,
                         Port(ioc, conn, objServer, bmcObjectPath, *portId));
        associations->emplace_back(
            Association{"port", "bmc", bmcPorts.at(*portId).getObjectPath()});
    }
}

void MetricCollector::registerAssociations()
{
    associations = std::make_shared<std::vector<Association>>();

    association_interface =
        objServer.add_interface(bmcObjectPath, associationInterfaceName);

    association_interface->register_property_r(
        "Associations", *associations,
        sdbusplus::vtable::property_::emits_change,
        [associations{associations}](const auto&) { return *associations; });

    fmt::print(stderr, "Registered property\n");
    association_interface->initialize();
}

void MetricCollector::removeAssociation(const std::string objectPath)
{
    for (auto it = associations->begin(); it != associations->end(); ++it)
    {
        auto association = *it;
        if (std::get<2>(association) == objectPath)
        {
            fmt::print(stderr, "Erased daemon {}\n", objectPath);
            std::swap(association, associations->back());
            associations->pop_back();
            return;
        }
    }
}

void MetricCollector::createOrUpdateDaemonObjectFromPid(const int pid)
{
    conn->async_method_call(
        [this, pid](const boost::system::error_code ec,
                    const sdbusplus::message::object_path& objPath) {
            if (ec)
            {
                if (bmcDaemons.contains(pid))
                {
                    bmcDaemons.erase(bmcDaemons.find(pid));
                }
                return;
            }

            // If object exists, then just update its daemon info
            if (bmcDaemons.contains(pid))
            {
                bmcDaemons.at(pid).updateInfo();
                return;
            }

            const std::string daemonObjectPath = objPath.str;

            // If not add it to the map
            bmcDaemons.emplace(pid, Daemon(ioc, conn, objServer, bmcObjectPath,
                                           daemonObjectPath, pid));

            associations->emplace_back(Association{
                "daemon", "bmc", bmcDaemons.at(pid).getObjectPath()});
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "GetUnitByPID",
        static_cast<uint32_t>(pid));
}

void MetricCollector::registerDaemons()
{

    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        std::optional<int> pid = isNumericPath(path);

        // Exclude sysinit unit because it is a unit but not a service
        if (pid == std::nullopt || *pid == 1)
        {
            continue;
        }

        createOrUpdateDaemonObjectFromPid(*pid);
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

        if (portId == std::nullopt)
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
            associations->emplace_back(Association{
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
        if (pid == std::nullopt || *pid == 1)
        {
            continue;
        }

        createOrUpdateDaemonObjectFromPid(*pid);
        allDaemonPids.emplace(*pid);
    }

    // Remove nonexistent daemons
    for (auto it = bmcDaemons.begin(); it != bmcDaemons.end();)
    {
        auto daemonPid = it->first;
        if (!allDaemonPids.contains(daemonPid))
        {
            fmt::print(stderr, "Removing pid {}\n", daemonPid);
            it = bmcDaemons.erase(it);
            removeAssociation(it->second.getObjectPath());
            continue;
        }
        ++it;
    }
}

void MetricCollector::update()
{
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
                fmt::print(stderr, "Cannot get BMC host properties\n");
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

            metrics->set_property("BootCount", bootCount);
            metrics->set_property("CrashCount", crashCount);
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

} // namespace

void metricCollectCallback(std::shared_ptr<boost::asio::deadline_timer> timer,
                           sdbusplus::bus_t& bus, MetricCollector& metricCol)
{
    timer->expires_from_now(boost::posix_time::seconds(TIMER_INTERVAL));
    timer->async_wait([timer, &bus,
                       &metricCol](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            fmt::print(stderr, "sensorRecreateTimer aborted: {}\n", ec.what());

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
    // Sleep for 3 minutes
    sleep(3);

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
