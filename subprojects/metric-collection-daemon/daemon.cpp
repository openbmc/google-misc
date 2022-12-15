#include "daemon.hpp"

#include "utils.hpp"

#include <fmt/format.h>
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
#include <string_view>

Daemon::Daemon(boost::asio::io_context& ioc,
               std::shared_ptr<sdbusplus::asio::connection> conn,
               sdbusplus::asio::object_server& objServer,
               const std::string bmcPath, const std::string daemonObjectPath,
               const int pid) :
    ioc(ioc),
    conn(conn), objServer(objServer), daemonObjectPath(daemonObjectPath),
    pid(pid)
{

    updateInfo();
    objectPath = fmt::format("{}/{}", bmcPath, pid);

    metrics = objServer.add_interface(objectPath, daemonInterfaceName);

    metrics->register_property("CommandLine",
                               fmt::format("{} - {}", commandLine, daemonId));
    metrics->register_property("KernelTimeSeconds", kernelSeconds);
    metrics->register_property("UserTimeSeconds", userSeconds);
    metrics->register_property("UptimeSeconds", uptime);
    metrics->register_property("ResidentSetSizeBytes", memoryUsage);
    metrics->register_property("NFileDescriptors", fileDescriptors);
    metrics->register_property("RestartCount", restartCount);

    metrics->initialize();
}

void Daemon::updateDaemonId()
{
    auto m = conn->new_method_call("org.freedesktop.systemd1",
                                   daemonObjectPath.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");
    m.append("org.freedesktop.systemd1.Unit", "Id");

    auto reply = conn->call(m);

    DbusVariantType daemonIdVariant;
    reply.read(daemonIdVariant);
    daemonId = std::get<std::string>(daemonIdVariant);
}

void Daemon::updateProcessStatistics()
{
    const std::string& statPath = fmt::format("/proc/{}/stat", pid);
    std::string_view processStats = readFileIntoString(statPath);

    const long ticksPerSec = getTicksPerSec();

    if (ticksPerSec <= 0)
    {
        fmt::print(stderr, "Ticks/Sec is < 0\n");
        return;
    }

    const float invTicksPerSec = 1.0f / static_cast<float>(ticksPerSec);

    // In the parse proc file:
    // Index 1: tcomm
    // Index 13: utime
    // Index 14: stime
    // Index 23: rss
    // Source: https://man7.org/linux/man-pages/man5/proc.5.html
    std::vector<std::string> parsedProcFile = split(processStats, ' ');

    // Get tcomm
    commandLine = parsedProcFile[1];

    // Add utime, stime
    size_t utimeTicks = toSizeT(parsedProcFile[13]);
    size_t stimeTicks = toSizeT(parsedProcFile[14]);

    userSeconds = static_cast<double>(utimeTicks) * invTicksPerSec;
    kernelSeconds = static_cast<double>(stimeTicks) * invTicksPerSec;

    memoryUsage = toSizeT(parsedProcFile[23]);
}

void Daemon::updateFdCount()
{
    const std::string& fdPath = fmt::format("/proc/{}/fd", pid);
    fileDescriptors = std::distance(std::filesystem::directory_iterator(fdPath),
                                    std::filesystem::directory_iterator{});
}

void Daemon::updateRestartCount()
{
    auto m = conn->new_method_call("org.freedesktop.systemd1",
                                   daemonObjectPath.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");
    m.append("org.freedesktop.systemd1.Service", "NRestarts");

    auto reply = conn->call(m);

    DbusVariantType restartCountVariant;
    reply.read(restartCountVariant);

    restartCount = std::get<uint32_t>(restartCountVariant);
}

void Daemon::updateUptime()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    float millisNow = static_cast<float>(tv.tv_sec) * 1000.0f +
                      static_cast<float>(tv.tv_usec) / 1000.0f;

    struct stat t_stat;
    stat((fmt::format("/proc/{}", pid)).c_str(), &t_stat);
    struct timespec st_atim = t_stat.st_atim;
    float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0f +
                        static_cast<float>(st_atim.tv_nsec) / 1000000.0f;

    uptime = (millisNow - millisCtime) / 1000.0f;
}

void Daemon::updateInfo()
{
    updateDaemonId();
    updateProcessStatistics();
    updateFdCount();
    updateRestartCount();
    updateUptime();
}

void Daemon::unregister()
{
    objServer.remove_interface(metrics);
}

std::string Daemon::getObjectPath()
{
    return objectPath;
}
