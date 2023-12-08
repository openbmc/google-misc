
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

const constexpr char* OperatingSystemService =
    "xyz.openbmc_project.State.OperatingSystem";
const constexpr char* OperatingSystemPath = "/xyz/openbmc_project/state/os";
const constexpr char* OperatingSystemStatusInterface =
    "xyz.openbmc_project.State.OperatingSystem.Status";
const constexpr char* OperatingSystemStateProperty = "OperatingSystemState";
const constexpr char* OperatingSystemStateStandby =
    "xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Standby";
const constexpr char* OperatingSystemStateInactive =
    "xyz.openbmc_project.State.OperatingSystem.Status.OSStatus.Inactive";
const constexpr char* BareMetalActiveTarget = "gbmc-bare-metal-active.target";

const constexpr char* SystemdService = "org.freedesktop.systemd1";
const constexpr char* SystemdManagerObject = "/org/freedesktop/systemd1";
const constexpr char* SystemdManagerInterface =
    "org.freedesktop.systemd1.Manager";

void setUnitStatus(sdbusplus::asio::connection& bus, bool status)
{
    auto method = bus.new_method_call(SystemdService, SystemdManagerObject,
                                      SystemdManagerInterface,
                                      status ? "StartUnit" : "StopUnit");
    method.append(BareMetalActiveTarget, "replace");

    bus.call(method);
}

/* This only gets called once on startup. */
void checkPostCompleteStartup(sdbusplus::asio::connection& bus)
{
    sdbusplus::asio::getProperty<std::string>(
        bus, OperatingSystemService, OperatingSystemPath,
        OperatingSystemStatusInterface, OperatingSystemStateProperty,
        [&](const boost::system::error_code& ec,
            const std::string& postCompleteState) {
            if (ec)
            {
                lg2::error("Error when checking Post Complete GPIO state");
                return;
            }

            lg2::info("Post Complete state is {STATE}", "STATE", postCompleteState);
            /*
             * If state is Standby, enable the bare-metal-active systemd
             * target.
             * If state is Inactive, no-op cause ipmi is enabled by default.
             */
            if (postCompleteState == OperatingSystemStateStandby)
            {
                setUnitStatus(bus, true);
            }
        }
    );
}

/* Gets called when a GPIO state change is detected. */
void checkPostCompleteEvent(sdbusplus::asio::connection& bus)
{
    sdbusplus::asio::getProperty<std::string>(
        bus, OperatingSystemService, OperatingSystemPath,
        OperatingSystemStatusInterface, OperatingSystemStateProperty,
        [&](const boost::system::error_code& ec,
            const std::string& postCompleteState) {
            if (ec)
            {
                lg2::error("Error when checking Post Complete GPIO state");
                return;
            }

            lg2::info("Post Complete state is {STATE}", "STATE", postCompleteState);
            /*
             * If state is Inactive, disable the bare-metal-active systemd
             * target to enable ipmi.
             */
            if (postCompleteState == OperatingSystemStateInactive)
            {
                setUnitStatus(bus, false);
            }
        }
    );
}

int main()
{
    try
    {
        /* Setup connection to dbus. */
        boost::asio::io_context io;
        auto conn = sdbusplus::asio::connection(io);

        /* check ipmi status at startup */
        checkPostCompleteStartup(conn);
        /*
         * Set up an event handler to process Post Complete GPIO state changes.
         */
        boost::asio::steady_timer filterTimer(io);

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(conn),
            std::format(
                "type='signal',member='PropertiesChanged',path_namespace='"
                "/xyz/openbmc_project/state/os',arg0namespace='{}'",
                OperatingSystemStatusInterface),
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
                filterTimer.expires_from_now(std::chrono::seconds(1));

                filterTimer.async_wait([&](const boost::system::error_code& ec) {
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
                     * Check if the Post Complete got deasserted stop the bare
                     * metal active target.
                     */
                    checkPostCompleteEvent(conn);
                });
            }
        );

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
