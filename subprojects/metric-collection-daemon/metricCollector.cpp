#include "metricCollector.hpp"

#include "utils.hpp"

#include <fmt/format.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <sstream>

extern "C"
{
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
}

#define TIMER_INTERVAL 10

MetricCollector::MetricCollector(
    boost::asio::io_context& ioc,
    std::shared_ptr<sdbusplus::asio::connection> conn,
    sdbusplus::asio::object_server& objServer) :
    ioc(ioc),
    conn(conn), objServer(objServer)
{
    updateBootCount();
    updateLatencyInfo();
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
        Port bmcPort(ioc, conn, objServer, bmcObjectPath, i);
        bmcPorts.emplace_back(bmcPort);
    }
}

void MetricCollector::registerAssociations()
{
    std::vector<Association> associations;

    addPortAssociations(associations);
    addDaemonAssociations(associations);

    auto interface =
        objServer.add_interface(bmcObjectPath, associationInterfaceName);
    interface->register_property("Associations", associations);

    interface->initialize();
}

void MetricCollector::addPortAssociations(
    std::vector<Association>& associations)
{
    for (auto& port : bmcPorts)
    {
        Association portAssociation{"port", "bmc", port.getObjectPath()};
        associations.emplace_back(portAssociation);
    }
}

void MetricCollector::addDaemonAssociations(
    std::vector<Association>& associations)
{
    for (auto& daemon : bmcDaemons)
    {
        Association daemonAssociation{"daemon", "bmc", daemon.getObjectPath()};
        associations.emplace_back(daemonAssociation);
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
    constexpr std::string_view procPath = "/proc/";

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

        bmcDaemons.emplace_back(Daemon(ioc, conn, objServer, bmcObjectPath,
                                       *daemonObjectPath, *pid));
    }
}

void MetricCollector::unregisterPorts()
{
    for (auto& port : bmcPorts)
    {
        port.unregister();
    }
    bmcPorts.clear();
}

void MetricCollector::unregisterDaemons()
{
    for (auto& daemon : bmcDaemons)
    {
        daemon.unregister();
    }
    bmcDaemons.clear();
}

void MetricCollector::update()
{
    unregisterPorts();
    unregisterDaemons();
    registerPorts();
    registerDaemons();
    updateLatencyInfo();
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
            std::cerr << bootinfo[0] << " " << bootinfo[1] << " " << bootinfo[2]
                      << std::endl;
            bool updated = false;
            for (const auto& it : properties)
            {
                // If last reboot time is different then what is stored,
                // then update info
                if (it.first == "LastRebootTime" &&
                    std::get<uint64_t>(it.second) != bootinfo[2])
                {
                    std::cerr << std::get<uint64_t>(it.second) << std::endl;
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

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

        if (family != AF_PACKET || ifa->ifa_data == NULL)
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

    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Add object manager through object_server
    sdbusplus::asio::object_server objectServer(conn);

    auto metricCol = std::make_shared<MetricCollector>(io, conn, objectServer);

    sdbusplus::asio::sd_event_wrapper sdEvents(io);

    auto metricCollectTimer = std::make_shared<boost::asio::deadline_timer>(io);

    // Start the timer
    io.post([conn, &metricCollectTimer, &metricCol]() {
        metricCollectCallback(metricCollectTimer, *conn, *metricCol);
    });

    io.run();

    return 0;
}
