
#include <boost/asio/deadline_timer.hpp>
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

/* This only gets called once on startup. */
void checkIfPostCompleteAsserted(sdbusplus::asio::connection& bus)
{
    auto handlePostCompleteState = [](const boost::system::error_code& ec,
                                      const std::string postCompleteState) {
        if (ec)
        {
            lg2::error("Error when checking Post Complete GPIO state");
            return;
        }

        lg2::info("Post Complete state is {STATE}", "STATE",
                  postCompleteState);
        /*
         * If state is Standby, enable the bare-metal-active systemd
         * target.
         */
    };

    sdbusplus::asio::getProperty(conn, OperatingSystemService,
                           OperatingSystemPath, OperatingSystemStatusInterface,
                           OperatingSystemStateProperty,
                           handlePostCompleteState);
}

/* Gets called when a GPIO state change is detected. */
void checkIfPostCompleteDeasserted(sdbusplus::asio::connection& bus)
{
    auto handlePostCompleteState = [](const boost::system::error_code& ec,
                                      const std::string postCompleteState) {
        if (ec)
        {
            lg2::error("Error when checking Post Complete GPIO state");
            return;
        }

        lg2::info("Post Complete state is {STATE}", "STATE",
                  postCompleteState);
        /*
         * If state is Inactive, disable the bare-metal-active systemd
         * target.
         */

    };

    sdbusplus::asio::getProperty(conn, OperatingSystemService,
                           OperatingSystemPath, OperatingSystemStatusInterface,
                           OperatingSystemStateProperty,
                           handlePostCompleteState);
}

int main(void)
{
    try
    {
        /* Setup connection to dbus. */
        boost::asio::io_context io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);

        boost::asio::post(io, [&]() {  });

        /*
         * Set up an event handler to process Post Complete GPIO state changes.
         */
        boost::asio::deadline_timer filterTimer(io);
        std::function<void(sdbusplus::message_t&)> eventHandler =
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
            filterTimer.expires_from_now(boost::posix_time::seconds(1));

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
                checkPostCompleteState(conn);
            });

        };

        auto match = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(*conn),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string("/xyz/openbmc_project/state/os") +
                "',arg0namespace='" + OperatingSystemStatusInterface + "'",
            eventHandler);

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
