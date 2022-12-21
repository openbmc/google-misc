#pragma once

#include "utils.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <charconv>
#include <cstdlib>
#include <iostream>

const std::string portInterfaceName = "xyz.openbmc_project.Metric.Port";

class Port
{

    enum class LinkState : size_t
    {
        LinkDown_Disabled = 0,
        LinkDown_Enabled,
        LinkUp_Disabled,
        LinkUp_Enabled,
    };

    std::string linkStateToString(LinkState state)
    {
        std::string linkStateString = "xyz.openbmc_project.Metric.LinkState";
        switch (state)
        {
            case LinkState::LinkDown_Disabled:
                linkStateString += ".LinkDown_Disabled";
                break;
            case LinkState::LinkDown_Enabled:
                linkStateString += ".LinkDown_Enabled";
                break;
            case LinkState::LinkUp_Disabled:
                linkStateString += ".LinkUp_Disabled";
                break;
            case LinkState::LinkUp_Enabled:
                linkStateString += ".LinkUp_Enabled";
                break;
        };
        return linkStateString;
    }

  public:
    Port(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
         sdbusplus::asio::object_server& objServer, const std::string bmcPath,
         const int portNum) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer), portNum_(portNum)
    {

        // Initialize object
        updateInfo();

        objectPath = bmcPath + "/" + portId;

        metrics_ = objServer.add_interface(objectPath, portInterfaceName);

        metrics_->register_property("CurrentSpeedGbps", speed);
        metrics_->register_property("RXDroppedPackets", rxDroppedPackets);
        metrics_->register_property("TXDroppedPackets", txDroppedPackets);
        metrics_->register_property("LinkState", linkStateToString(linkState));

        metrics_->initialize();
    };

    size_t getSpeed(std::string interfaceName)
    {
        const std::string& speedPath =
            "/sys/class/net/" + interfaceName + "/speed";
        const std::string speedFile = readFileIntoString(speedPath);
        size_t speed;
        auto res = std::from_chars(speedFile.data(),
                                   speedFile.data() + speedFile.size(), speed);
        if (res.ec != std::errc())
        {
            speed = 0;
        }
        return speed;
    };

    bool getEnabled(std::string interfaceName)
    {
        const std::string& dormantPath =
            "/sys/class/net/" + interfaceName + "/dormant";
        const std::string dormant = readFileIntoString(dormantPath);
        // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net

        int enabled;
        auto res = std::from_chars(dormant.data(),
                                   dormant.data() + dormant.size(), enabled);
        if (res.ec != std::errc())
        {
            enabled = 0;
        }
        return enabled == 0;
    };

    bool getLinkUp(std::string interfaceName)
    {
        const std::string& linkUpPath =
            "/sys/class/net/" + interfaceName + "/carrier";
        const std::string linkUp = readFileIntoString(linkUpPath);

        int carrier;
        auto res = std::from_chars(linkUp.data(), linkUp.data() + linkUp.size(),
                                   carrier);
        if (res.ec != std::errc())
        {
            carrier = 0;
        }
        // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
        return carrier != 0;
    };

    LinkState getLinkState(std::string interfaceName)
    {
        bool linkUp = getLinkUp(interfaceName);
        bool enabled = getEnabled(interfaceName);
        return static_cast<LinkState>(2 * (linkUp ? 1 : 0) + (enabled ? 1 : 0));
    }

    void updateInfo()
    {
        struct ifaddrs* ifaddr;
        int family;

        if (getifaddrs(&ifaddr) == -1)
        {
            return;
        }

        int curPortNum = 0;

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

            if (curPortNum == portNum_)
            {
                struct rtnl_link_stats* stats =
                    static_cast<rtnl_link_stats*>(ifa->ifa_data);
                portId = ifa->ifa_name;
                speed = getSpeed(portId);
                linkState = getLinkState(portId);
                txDroppedPackets = static_cast<size_t>(stats->tx_dropped);
                rxDroppedPackets = static_cast<size_t>(stats->rx_dropped);
                freeifaddrs(ifaddr);
                return;
            }

            // Only track AF_PACKETS
            ++curPortNum;
        }
        freeifaddrs(ifaddr);

        return;
    };

    void unregister()
    {
        objServer_.remove_interface(metrics_);
    }

    const std::string getObjectPath()
    {
        return objectPath;
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;
    std::string objectPath;
    const int portNum_;
    std::shared_ptr<sdbusplus::asio::dbus_interface> metrics_;

    std::string portId;
    LinkState linkState;
    size_t txDroppedPackets;
    size_t rxDroppedPackets;
    size_t speed;
};