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
    objectPath = fmt::format("{}/{}", bmcPath, pid);

    metrics = objServer.add_interface(objectPath, daemonInterfaceName);

    // These are dummies now and will be updated by async callbacks
    updateDaemonId();
    updateCommandLine();

    // Initialize them to be dummy for now and set them as found
    metrics->register_property_r(
        "CommandLine", fmt::format("{} - {}", *commandLine, *daemonId),
        sdbusplus::vtable::property_::emits_change,
        [commandLine{commandLine}, daemonId{daemonId}](const auto&) {
            return fmt::format("{} - {}", *commandLine, *daemonId);
        });
    metrics->register_property("KernelTimeSeconds", kernelSeconds);
    metrics->register_property("UserTimeSeconds", userSeconds);
    metrics->register_property("UptimeSeconds", uptime);
    metrics->register_property("ResidentSetSizeBytes", memoryUsage);
    metrics->register_property("NFileDescriptors", fileDescriptors);
    metrics->register_property("RestartCount", restartCount);

    metrics->initialize();

    updateInfo();
}

Daemon::~Daemon()
{
    objServer.remove_interface(metrics);
}

void Daemon::updateDaemonId()
{
    daemonId = std::make_shared<std::string>("");
    conn->async_method_call(
        [daemonId{daemonId}](const boost::system::error_code ec,
                             const DbusVariantType& id) {
            if (ec)
            {
                fmt::print(stderr, "Failed to get daemon Id {}\n", ec.what());
                return;
            }
            daemonId->append(std::get<std::string>(id));
        },
        "org.freedesktop.systemd1", daemonObjectPath.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Unit", "Id");
}

void Daemon::updateProcessStatistics()
{
    try
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

        // In the parse proc file (split combines first 2 elements of proc
        // file): Index 12: utime Index 13: stime Index 22: rss Source:
        // https://man7.org/linux/man-pages/man5/proc.5.html
        std::vector<std::string> parsedProcFile = split(processStats, ' ');

        // Add utime, stime
        size_t utimeTicks = toSizeT(parsedProcFile.at(12));
        size_t stimeTicks = toSizeT(parsedProcFile.at(13));

        userSeconds = static_cast<double>(utimeTicks) * invTicksPerSec;
        kernelSeconds = static_cast<double>(stimeTicks) * invTicksPerSec;

        // The RSS here is in sets of pages. Each page is 4KB.
        memoryUsage = toSizeT(parsedProcFile.at(22)) * PAGE_SIZE;

        metrics->set_property("KernelTimeSeconds", kernelSeconds);
        metrics->set_property("UserTimeSeconds", userSeconds);
        metrics->set_property("ResidentSetSizeBytes", memoryUsage);
    }
    catch (std::exception& e)
    {
        fmt::print(stderr, "Error reading proc file {}\n", e.what());
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
        metrics->set_property("NFileDescriptors", fileDescriptors);
    }
    catch (std::exception& e)
    {
        fmt::print("Failed to open file descriptors for pid {}: {}\n", pid,
                   e.what());
    }
}

void Daemon::updateRestartCount()
{
    conn->async_method_call(
        [metrics{metrics}](const boost::system::error_code ec,
                                 const DbusVariantType& nRestarts) {
            if (ec)
            {
                fmt::print(stderr, "Failed to get restart count {}\n",
                           ec.what());
                return;
            }
            metrics->set_property("RestartCount", std::get<uint32_t>(nRestarts));
        },
        "org.freedesktop.systemd1", daemonObjectPath.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Service", "NRestarts");
}

void Daemon::updateUptime()
{
    float millisNow =
        std::chrono::high_resolution_clock::now().time_since_epoch() /
        std::chrono::milliseconds(1);

    struct stat t_stat;
    stat((fmt::format("/proc/{}", pid)).c_str(), &t_stat);
    struct timespec st_atim = t_stat.st_atim;
    float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0f +
                        static_cast<float>(st_atim.tv_nsec) / 1000000.0f;

    uptime = (millisNow - millisCtime) / 1000.0f;

    metrics->set_property("UptimeSeconds", uptime);
}

void Daemon::updateInfo()
{
    updateProcessStatistics();
    updateFdCount();
    updateRestartCount();
    updateUptime();
}

void Daemon::updateCommandLine()
{
    commandLine = std::make_shared<std::string>("");

    conn->async_method_call(
        [commandLine{commandLine}](const boost::system::error_code ec,
                                   const ExecStartVariantType& execStart) {
            if (ec)
            {
                fmt::print(stderr, "Failed to get commandLine {}\n", ec.what());
                return;
            }
            commandLine->append(
                std::get<0>(std::get<ExecStartType>(execStart)[0]));
        },
        "org.freedesktop.systemd1", daemonObjectPath.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Service", "ExecStart");
}

std::string Daemon::getObjectPath() const
{
    return objectPath;
}
