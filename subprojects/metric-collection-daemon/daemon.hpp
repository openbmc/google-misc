#pragma once

#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include <iostream>
#include <string>

const std::string daemonInterfaceName = "xyz.openbmc_project.Metric.Daemon";

class Daemon
{
  public:
    Daemon(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
           sdbusplus::asio::object_server& objServer, const std::string bmcPath,
           const std::string daemonObjectPath, const int pid) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer), daemonObjectPath_(daemonObjectPath),
        pid_(pid)
    {

        updateInfo();
        objectPath = bmcPath + "/" + std::to_string(pid);

        metrics_ = objServer.add_interface(objectPath, daemonInterfaceName);

        metrics_->register_property("CommandLine",
                                    (commandLine + " - " + daemonId));
        metrics_->register_property("KernelTimeSeconds", kernelSeconds);
        metrics_->register_property("UserTimeSeconds", userSeconds);
        metrics_->register_property("UptimeSeconds", uptime);
        metrics_->register_property("ResidentSetSizeBytes", memoryUsage);
        metrics_->register_property("NFileDescriptors", fileDescriptors);
        metrics_->register_property("RestartCount", restartCount);

        metrics_->initialize();

    };

    void updateDaemonId()
    {
        auto m = bus_.new_method_call("org.freedesktop.systemd1",
                                      daemonObjectPath_.c_str(),
                                      "org.freedesktop.DBus.Properties", "Get");
        m.append("org.freedesktop.systemd1.Unit", "Id");

        auto reply = bus_.call(m);

        DbusVariantType daemonIdVariant;
        reply.read(daemonIdVariant);
        daemonId = std::get<std::string>(daemonIdVariant);
    }

    void updateProcessStatistics()
    {
        const std::string& statPath = "/proc/" + std::to_string(pid_) + "/stat";
        const std::string processStats = readFileIntoString(statPath);

        const long ticksPerSec = getTicksPerSec();

        if (ticksPerSec <= 0)
        {
            std::cerr << "Ticks/Sec is < 0" << std::endl;
        }

        const float invTicksPerSec = 1.0f / static_cast<float>(ticksPerSec);

        std::string temp(processStats);
        char* pCol = strtok(temp.data(), " ");

        std::array<uint, 4> fields = {1, 13, 14,
                                      23}; // tcomm, utime, stime, rss
        uint fieldIdx = 0;

        if (pCol == nullptr)
        {
            return;
        }

        for (uint colIdx = 0; colIdx < 24; ++colIdx)
        {
            if (fieldIdx >= fields.size() || colIdx != fields[fieldIdx])
            {
                pCol = strtok(nullptr, " ");
                continue;
            }

            switch (fieldIdx)
            {
                case 0:
                {
                    commandLine = std::string(pCol);
                    break;
                }
                case 1:
                    [[fallthrough]];
                case 2:
                {
                    int ticks;
                    auto res =
                        std::from_chars(pCol, pCol + sizeof(pCol), ticks);

                    if (res.ec != std::errc())
                    {
                        ticks = 0;
                    }
                    double t = static_cast<double>(ticks) * invTicksPerSec;

                    if (fieldIdx == 1)
                    {
                        userSeconds = t;
                    }
                    else if (fieldIdx == 2)
                    {
                        kernelSeconds = t;
                    }
                    break;
                }
                case 3:
                {
                    auto res =
                        std::from_chars(pCol, pCol + sizeof(pCol), memoryUsage);
                    if (res.ec != std::errc())
                    {
                        memoryUsage = 0;
                    }
                }
            }
            ++fieldIdx;
            pCol = strtok(nullptr, " ");
        }
    }

    void updateFdCount()
    {
        const std::string& fdPath = "/proc/" + std::to_string(pid_) + "/fd";
        fileDescriptors =
            std::distance(std::filesystem::directory_iterator(fdPath),
                          std::filesystem::directory_iterator{});
    }

    void updateRestartCount()
    {
        auto m = bus_.new_method_call("org.freedesktop.systemd1",
                                      daemonObjectPath_.c_str(),
                                      "org.freedesktop.DBus.Properties", "Get");
        m.append("org.freedesktop.systemd1.Service", "NRestarts");

        auto reply = bus_.call(m);

        DbusVariantType restartCountVariant;
        reply.read(restartCountVariant);

        restartCount = std::get<uint32_t>(restartCountVariant);
    }

    void updateUptime()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        float millisNow = static_cast<float>(tv.tv_sec) * 1000.0f +
                          static_cast<float>(tv.tv_usec) / 1000.0f;

        struct stat t_stat;
        stat(("/proc/" + std::to_string(pid_)).c_str(), &t_stat);
        struct timespec st_atim = t_stat.st_atim;
        float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0f +
                            static_cast<float>(st_atim.tv_nsec) / 1000000.0f;

        uptime = (millisNow - millisCtime) / 1000.0f;
    }

    void updateInfo()
    {
        updateDaemonId();
        updateProcessStatistics();
        updateFdCount();
        updateRestartCount();
        updateUptime();
    }

    void unregister()
    {
        objServer_.remove_interface(metrics_);
    }

    std::string getObjectPath()
    {
        return objectPath;
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    const std::string daemonObjectPath_;
    const int pid_;
    std::string objectPath;
    std::shared_ptr<sdbusplus::asio::dbus_interface> metrics_;

    std::string daemonId;
    std::string commandLine;
    double kernelSeconds;
    double userSeconds;
    double uptime;
    size_t memoryUsage;
    size_t fileDescriptors;
    size_t restartCount;
};