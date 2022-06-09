#include "btStateMachine.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server.hpp>

#include <future>
#include <iostream>
#include <memory>

constexpr const bool debug = true;

int main()
{

    // setup connection to dbus
    boost::asio::io_service io;
    std::shared_ptr<sdbusplus::asio::connection> conn =
        std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name("com.google.gbmc.btmanager");
    sdbusplus::asio::object_server server =
        sdbusplus::asio::object_server(conn);
    sdbusplus::bus::bus& bus = static_cast<sdbusplus::bus::bus&>(*conn);

    // BTStateMachine initialization
    std::string hostState = "Off";
    auto method = bus.new_method_call("xyz.openbmc_project.State.Host",
                                      "/xyz/openbmc_project/state/host0",
                                      "org.freedesktop.DBus.Properties", "Get");
    method.append("xyz.openbmc_project.State.Host", "CurrentHostState");
    try
    {
        std::variant<std::string> value{};
        auto reply = bus.call(method);
        reply.read(value);
        hostState = std::get<std::string>(value);
        if (debug)
        {
            std::cerr << "[DEBUG]: hostState is " << hostState << std::endl;
        }
    }
    catch (const sdbusplus::exception::exception& ex)
    {
        std::cerr << "[ERROR]: Get CurrentHostState failed" << std::endl;
    }

    // We don't expect hostState is the value other than `Off` and `Running`
    if (!boost::ends_with(hostState, "Running"))
    {
        hostState = "Off";
    }

    auto btsm = std::make_shared<BTStateMachine>(
        boost::ends_with(hostState, "Running"));

    // Monitor host state change
    std::function<void(sdbusplus::message::message & message)> eventHandler =
        [&hostState, &btsm](sdbusplus::message::message& message) {
            std::string objectName;
            boost::container::flat_map<
                std::string,
                std::variant<std::string, bool, int64_t, uint64_t, double>>
                values;
            message.read(objectName, values);
            auto findState = values.find("CurrentHostState");

            if (debug)
            {
                std::cerr << "[DEBUG]: CurrentHostState changed, from = "
                          << hostState << ", to = "
                          << std::get<std::string>(findState->second)
                          << std::endl;
            }

            if (findState != values.end())
            {
                if (boost::ends_with(hostState, "Running") &&
                    boost::ends_with(std::get<std::string>(findState->second),
                                     "Off"))
                {
                    hostState = "Off";
                    btsm->next(BTTimePoint::kOSKernelDownEnd);
                }
                else if (boost::ends_with(hostState, "Off") &&
                         boost::ends_with(
                             std::get<std::string>(findState->second),
                             "Running"))
                {
                    hostState = "Running";
                    btsm->next(BTTimePoint::kBIOSStart);
                }
            }
        };

    sdbusplus::bus::match::match powerMatch = sdbusplus::bus::match::match(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/host0",
            "xyz.openbmc_project.State.Host"),
        eventHandler);

    // Start
    io.run();

    return 0;
}
