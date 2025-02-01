
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "host_gpio_monitor_conf.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <format>

ABSL_FLAG(std::string, host_label, "0",
          "Label for the host in question. Usually this is an integer.");

const constexpr char* OperatingSystemStateInactive =
    "xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Inactive";
const constexpr char* BareMetalActiveTargetTemplate =
    "gbmc-bare-metal-active@{}.target";

const constexpr char* SystemdService = "org.freedesktop.systemd1";
const constexpr char* SystemdManagerObject = "/org/freedesktop/systemd1";
const constexpr char* SystemdManagerInterface =
    "org.freedesktop.systemd1.Manager";

void setUnitStatus(sdbusplus::asio::connection& bus, bool status,
                   const std::string& host_instance)
{
    auto method = bus.new_method_call(SystemdService, SystemdManagerObject,
                                      SystemdManagerInterface,
                                      status ? "StartUnit" : "StopUnit");
    method.append(std::format(BareMetalActiveTargetTemplate, host_instance),
                  "replace");

    bus.call(method);
}

void checkPostComplete(sdbusplus::asio::connection& bus,
                       const std::string& state, bool action,
                       const std::string& host_instance)
{
    sdbusplus::asio::getProperty<std::string>(
        bus, std::format(DBUS_SERVICE_NAME, host_instance),
        std::format(DBUS_OBJECT_PATH, host_instance), DBUS_INTERFACE,
        DBUS_PROPERTY_NAME,
        [&, state, action](const boost::system::error_code& ec,
                           const std::string& postCompleteState) {
            if (ec)
            {
                lg2::error("Error when checking Post Complete GPIO state");
                return;
            }

            lg2::info("Post Complete state is {STATE}", "STATE",
                      postCompleteState);

            /*
             * If the host OS is running (e.g. OperatingSystemState is Standby),
             * enable the bare-metal-active systemd target.
             * If the host CPU is in reset (e.g. OperatingSystemState is
             * Inactive), no-op cause IPMI is enabled by default.
             */
            if (postCompleteState == state)
            {
                setUnitStatus(bus, action, host_instance);
            }
        });
}

/* This only gets called once on startup. */
void checkPostCompleteStartup(sdbusplus::asio::connection& bus,
                              const std::string& host_instance)
{
    checkPostComplete(bus, DBUS_PROPERTY_HOST_RUNNING_VALUE, true,
                      host_instance);
}

/* Gets called when a GPIO state change is detected. */
void checkPostCompleteEvent(sdbusplus::asio::connection& bus,
                            const std::string& host_instance)
{
    checkPostComplete(bus, DBUS_PROPERTY_HOST_IN_RESET_VALUE, false,
                      host_instance);
}

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    std::string host_label = absl::GetFlag(FLAGS_host_label);

    try
    {
        /* Setup connection to dbus. */
        boost::asio::io_context io;
        auto conn = sdbusplus::asio::connection(io);

        /* check IPMI status at startup */
        checkPostCompleteStartup(conn, host_label);

        /* Notify that the service is done starting up. */
        sd_notify(0, "READY=1");

        /*
         * Set up an event handler to process Post Complete GPIO state changes.
         */
        boost::asio::steady_timer filterTimer(io);

        /*
         * Prepare object path we want to monitor, substituting in the host
         * label, if needed.
         */
        std::string objectPath = std::format(DBUS_OBJECT_PATH, host_label);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(conn),
            std::format(
                "type='signal',member='PropertiesChanged',path_namespace='"
                "{}',arg0namespace='{}'",
                objectPath, DBUS_INTERFACE),
            [&](sdbusplus::message_t& message) {
                if (message.is_method_error())
                {
                    lg2::error("eventHandler callback method error");
                    return;
                }

                /*
                 * This implicitly cancels the timer, if it's already pending.
                 * If there's a burst of events within a short period, we want
                 * to handle them all at once. So, we will wait this long for no
                 * more events to occur, before processing them.
                 */
                filterTimer.expires_from_now(std::chrono::milliseconds(100));

                filterTimer.async_wait(
                    [&](const boost::system::error_code& ec) {
                        if (ec == boost::asio::error::operation_aborted)
                        {
                            /* we were canceled */
                            return;
                        }
                        if (ec)
                        {
                            lg2::error("timer error");
                            return;
                        }

                        /*
                         * Stop the bare metal active target if the post
                         * complete got deasserted.
                         */
                        checkPostCompleteEvent(conn, host_label);
                    });
            });

        io.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        lg2::error(e.what(), "REDFISH_MESSAGE_ID",
                   std::string("OpenBMC.1.0.ServiceException"));

        return 2;
    }
    return 1;
}
