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

#include <charconv>
#include <chrono>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

Daemon::Daemon(boost::asio::io_context& ioc,
               std::shared_ptr<sdbusplus::asio::connection> conn,
               sdbusplus::asio::object_server& objServer,
               const std::string& bmcPath, const std::string& daemonObjectPath,
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

Daemon::~Daemon()
{
    objServer.remove_interface(metrics);
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

    // In the parse proc file (split combines first 2 elements of proc file):
    // Index 12: utime
    // Index 13: stime
    // Index 22: rss
    // Source: https://man7.org/linux/man-pages/man5/proc.5.html
    try
    {
        std::vector<std::string> parsedProcFile = split(processStats, ' ');

        // Add utime, stime
        size_t utimeTicks = toSizeT(parsedProcFile[12]);
        size_t stimeTicks = toSizeT(parsedProcFile[13]);

        userSeconds = static_cast<double>(utimeTicks) * invTicksPerSec;
        kernelSeconds = static_cast<double>(stimeTicks) * invTicksPerSec;

        // The RSS here is in sets of pages. Each page is 4KB.
        memoryUsage = toSizeT(parsedProcFile[22]) * PAGE_SIZE;
    }
    catch (std::exception& e)
    {
        fmt::print(stderr, "Error reading proc file {}", e.what());
    }
}

void Daemon::updateFdCount()
{
    try
    {
        const std::string& fdPath = fmt::format("/proc/{}/fd", pid);
        fileDescriptors =
            std::distance(std::filesystem::directory_iterator(fdPath),
                          std::filesystem::directory_iterator{});
    }
    catch (std::exception& e)
    {
        fmt::print("Failed to open file descriptors for pid {}: {}\n", pid,
                   e.what());
    }
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
    float millisNow = std::chrono::system_clock::now().time_since_epoch() /
                      std::chrono::milliseconds(1);

    struct stat t_stat;
    stat((fmt::format("/proc/{}", pid)).c_str(), &t_stat);
    struct timespec st_atim = t_stat.st_atim;
    float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0f +
                        static_cast<float>(st_atim.tv_nsec) / 1000000.0f;

    uptime = (millisNow - millisCtime) / 1000.0f;
}

void Daemon::updateDaemonProperties()
{
    if (metrics == nullptr)
    {
        return;
    }

    metrics->set_property("KernelTimeSeconds", kernelSeconds);
    metrics->set_property("UserTimeSeconds", userSeconds);
    metrics->set_property("UptimeSeconds", uptime);
    metrics->set_property("ResidentSetSizeBytes", memoryUsage);
    metrics->set_property("NFileDescriptors", fileDescriptors);
    metrics->set_property("RestartCount", restartCount);
}

void Daemon::updateInfo()
{
    fmt::print(stderr, "Updating daemon {}\n", pid);
    updateDaemonId();
    fmt::print(stderr, "Daemon id is {}\n", daemonId);
    updateCommandLine();
    updateProcessStatistics();
    updateFdCount();
    updateRestartCount();
    updateUptime();
    updateDaemonProperties();
}

void Daemon::updateCommandLine()
{
    auto m = conn->new_method_call("org.freedesktop.systemd1",
                                   daemonObjectPath.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");
    m.append("org.freedesktop.systemd1.Service", "ExecStart");

    auto reply = conn->call(m);

    ExecStartVariantType execStartVariant;
    try
    {
        reply.read(execStartVariant);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        fmt::print(stderr, "failed to unpack; sig is {}; error: {}\n",
                   reply.get_signature(), e.what());
    }

    commandLine = std::get<0>(std::get<ExecStartType>(execStartVariant)[0]);
}

std::string Daemon::getObjectPath() const
{
    return objectPath;
}
