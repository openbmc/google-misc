#include "port.hpp"

#include "utils.hpp"

#include <arpa/inet.h>
#include <fmt/format.h>
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
#include <string>
#include <string_view>

std::string Port::linkStateToString(LinkState state)
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

Port::Port(boost::asio::io_context& ioc,
           std::shared_ptr<sdbusplus::asio::connection> conn,
           sdbusplus::asio::object_server& objServer, const std::string bmcPath,
           const int portNum) :
    ioc(ioc),
    conn(conn), objServer(objServer), portNum(portNum)
{
    // Initialize object
    updatePortInfo();

    objectPath = fmt::format("{}/{}", bmcPath, portId);

    metrics = objServer.add_interface(objectPath, portInterfaceName);

    metrics->register_property("CurrentSpeedGbps", speed);
    metrics->register_property("RXDiscards", rxDroppedPackets);
    metrics->register_property("TXDiscards", txDroppedPackets);
    metrics->register_property("LinkState", linkStateToString(linkState));

    metrics->initialize();
};

size_t Port::getSpeed(std::string interfaceName)
{
    const std::string& speedPath =
        fmt::format("/sys/class/net/{}/speed", interfaceName);
    std::string_view speedFile = readFileIntoString(speedPath);

    return toSizeT(speedFile);
};

bool Port::getEnabled(std::string interfaceName)
{
    const std::string& dormantPath =
        fmt::format("/sys/class/net/{}/dormant", interfaceName);
    std::string_view dormant = readFileIntoString(dormantPath);
    // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net

    return toSizeT(dormant) == 0;
};

bool Port::getLinkUp(std::string interfaceName)
{
    const std::string& linkUpPath =
        fmt::format("/sys/class/net/{}/carrier", interfaceName);
    std::string_view linkUp = readFileIntoString(linkUpPath);

    // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
    return toSizeT(linkUp) != 0;
};

Port::LinkState Port::getLinkState(std::string interfaceName)
{
    bool linkUp = getLinkUp(interfaceName);
    bool enabled = getEnabled(interfaceName);
    return static_cast<LinkState>(2 * (linkUp ? 1 : 0) + (enabled ? 1 : 0));
}

void Port::updatePortInfo()
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

        if (curPortNum == portNum)
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

void Port::unregister()
{
    objServer.remove_interface(metrics);
}

const std::string Port::getObjectPath()
{
    return objectPath;
}