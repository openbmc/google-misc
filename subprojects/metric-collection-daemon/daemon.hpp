#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <string>

const std::string daemonInterfaceName = "xyz.openbmc_project.Metric.Daemon";

class Daemon
{
  public:
    Daemon(boost::asio::io_context& ioc,
           std::shared_ptr<sdbusplus::asio::connection> conn,
           sdbusplus::asio::object_server& objServer, const std::string bmcPath,
           const std::string daemonObjectPath, const int pid);

    void updateDaemonId();
    void updateProcessStatistics();
    void updateFdCount();
    void updateRestartCount();
    void updateUptime();
    void updateInfo();

    void unregister();

    std::string getObjectPath();

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server& objServer;

    const std::string daemonObjectPath;
    const int pid;
    std::string objectPath;
    std::shared_ptr<sdbusplus::asio::dbus_interface> metrics;

    std::string daemonId;
    std::string commandLine;
    double kernelSeconds;
    double userSeconds;
    double uptime;
    size_t memoryUsage;
    size_t fileDescriptors;
    size_t restartCount;
};