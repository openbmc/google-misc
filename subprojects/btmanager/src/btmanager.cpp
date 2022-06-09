#include "btStateMachine.hpp"
#include "dbusHandler.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>

#include <future>
#include <iostream>
#include <memory>

static constexpr bool debug = true;

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
    std::promise<std::string> p;
    std::future<std::string> f = p.get_future();
    sdbusplus::asio::getProperty<std::string>(
        *conn, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0", "xyz.openbmc_project.State.Host",
        "CurrentHostState",
        [&](boost::system::error_code ec, std::string property) {
            if (ec)
            {
                std::cerr << "Error: Can't obtain CurrentHostState"
                          << std::endl;
                return;
            }
            p.set_value(property);
        });

    std::string hostState = f.get();
    if (debug)
    {
        std::cerr << "[INFO]: btmanager init, host state = " << hostState
                  << std::endl;
    }
    // We don't expect hostState is the value other than `Off` and `Running`
    if (!boost::ends_with(hostState, "Running"))
    {
        hostState = "Off";
    }
    auto btsm = std::make_shared<BTStateMachine>(
        boost::ends_with(hostState, "Running"));

    // DbusHandler initialization
    DbusHandler dh(bus, "/xyz/openbmc_project/Time/Boot/host0", btsm);

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
                std::cerr << "[INFO]: CurrentHostState changed, from = "
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
        "type='signal',interface='org.freedesktop.DBus.Properties',path='/xyz/"
        "openbmc_project/state/"
        "host0',arg0='xyz.openbmc_project.State.Host'",
        eventHandler);

    // Start
    io.run();

    return 0;
}
