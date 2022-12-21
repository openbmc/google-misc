#include "metricCollector.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

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

void metricCollectCallback(std::shared_ptr<boost::asio::deadline_timer> timer,
                           sdbusplus::bus_t& bus, MetricCollector& metricCol)
{
    timer->expires_from_now(boost::posix_time::seconds(TIMER_INTERVAL));
    timer->async_wait(
        [timer, &bus, &metricCol](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                printf("sensorRecreateTimer aborted");
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

    auto metricCol = std::make_shared<MetricCollector>(io, *conn, objectServer);

    sdbusplus::asio::sd_event_wrapper sdEvents(io);

    auto metricCollectTimer = std::make_shared<boost::asio::deadline_timer>(io);

    // Start the timer
    io.post([conn, &metricCollectTimer, &metricCol]() {
        metricCollectCallback(metricCollectTimer, *conn, *metricCol);
    });

    io.run();
    
    return 0;
}
