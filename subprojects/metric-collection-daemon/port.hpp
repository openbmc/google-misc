#pragma once

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

const std::string portInterfaceName = "xyz.openbmc_project.Metric.Port";

class Port
{

    enum class LinkState : uint8_t
    {
        LinkDown_Disabled = 0,
        LinkDown_Enabled,
        LinkUp_Disabled,
        LinkUp_Enabled,
    };

    std::string linkStateToString(LinkState state);

  public:
    Port(boost::asio::io_context& ioc,
         std::shared_ptr<sdbusplus::asio::connection> conn,
         sdbusplus::asio::object_server& objServer, const std::string bmcPath,
         const int portNum);

    size_t getSpeed(std::string interfaceName);
    bool getEnabled(std::string interfaceName);
    bool getLinkUp(std::string interfaceName);
    LinkState getLinkState(std::string interfaceName);
    void updatePortInfo();

    void unregister() const;

    const std::string getObjectPath();

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server& objServer;
    std::string objectPath;
    const int portNum;
    std::shared_ptr<sdbusplus::asio::dbus_interface> metrics;

    std::string portId;
    LinkState linkState;
    size_t txDroppedPackets;
    size_t rxDroppedPackets;
    size_t speed;
};