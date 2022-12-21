#include "daemon.hpp"
#include "port.hpp"
#include "utils.hpp"

#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <variant>
#include <vector>

const std::string serviceName = "xyz.openbmc_project.Metric";
const std::string bmcObjectPath = "/xyz/openbmc_project/metric/bmc0";
const std::string bmcInterfaceName = "xyz.openbmc_project.Metric.BMC";
const std::string associationInterfaceName =
    "xyz.openbmc_project.Association.Definitions";

class MetricCollector
{
  public:
    MetricCollector(boost::asio::io_context& ioc,
                    sdbusplus::asio::connection& bus,
                    sdbusplus::asio::object_server& objServer) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer)
    {
        updateBootCount();
        std::cerr << "Register ports" << std::endl;
        registerPorts();
        std::cerr << "Register daemons" << std::endl;
        registerDaemons();
        std::cerr << "Register associations" << std::endl;
        registerAssociations();

        auto metrics = objServer.add_interface(bmcObjectPath, bmcInterfaceName);

        metrics->register_property("BootCount", bootCount);
        metrics->register_property("CrashCount", crashCount);
        metrics->initialize();
    }

    void registerPorts()
    {
        int numPorts = getNumPorts();

        for (int i = 0; i < numPorts; ++i)
        {
            Port bmcPort(ioc_, bus_, objServer_, bmcObjectPath, i);
            bmcPorts.emplace_back(bmcPort);
        }
    }

    void registerAssociations()
    {
        std::vector<Association> associations;

        addPortAssociations(associations);
        addDaemonAssociations(associations);

        auto interface =
            objServer_.add_interface(bmcObjectPath, associationInterfaceName);
        interface->register_property("Associations", associations);

        interface->initialize();
    }

    void addPortAssociations(std::vector<Association>& associations)
    {
        for (auto& port : bmcPorts)
        {
            Association portAssociation{"port", "bmc", port.getObjectPath()};
            associations.emplace_back(portAssociation);
        }
    }

    void addDaemonAssociations(std::vector<Association>& associations)
    {
        for (auto& daemon : bmcDaemons)
        {
            Association daemonAssociation{"daemon", "bmc",
                                          daemon.getObjectPath()};
            associations.emplace_back(daemonAssociation);
        }
    }

    std::optional<std::string> getDaemonObjectPathFromPid(const int pid)
    {
        try
        {
            auto m = bus_.new_method_call(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager", "GetUnitByPID");
            m.append(static_cast<uint32_t>(pid));

            auto reply = bus_.call(m);

            sdbusplus::message::object_path objectPathVariant;
            reply.read(objectPathVariant);

            return objectPathVariant.str;
        }
        catch (const std::exception e)
        {
            return std::nullopt;
        }
    }

    void registerDaemons()
    {
        constexpr std::string_view procPath = "/proc/";

        for (const auto& procEntry :
             std::filesystem::directory_iterator(procPath))
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

            Daemon bmcDaemon(ioc_, bus_, objServer_, bmcObjectPath,
                             *daemonObjectPath, *pid);
            bmcDaemons.emplace_back(bmcDaemon);
        }
    }

    void unregisterPorts()
    {
        for (auto& port : bmcPorts)
        {
            port.unregister();
        }
        bmcPorts.clear();
    }

    void unregisterDaemons()
    {
        for (auto& daemon : bmcDaemons)
        {
            daemon.unregister();
        }
        bmcDaemons.clear();
    }

    void update()
    {
        unregisterPorts();
        unregisterDaemons();
        registerPorts();
        registerDaemons();
    }

    void updateBootCount()
    {
        auto m = bus_.new_method_call(
            "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
            "org.freedesktop.DBus.Properties", "GetAll");

        m.append("xyz.openbmc_project.State.BMC");

        auto reply = bus_.call(m);

        DBusPropertiesMap properties;
        reply.read(properties);

        std::array<uint64_t, 3> bootinfo = parseBootInfo();
        std::cerr << bootinfo[0] << " " << bootinfo[1] << " " << bootinfo[2] << std::endl;
        bool updated = false;
        for (const auto& it : properties)
        {
            // If last reboot time is different then what is stored, then
            // update
            // info
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
    }

    int getNumPorts()
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

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    size_t bootCount;
    size_t crashCount;
    std::vector<Port> bmcPorts;
    std::vector<Daemon> bmcDaemons;
};