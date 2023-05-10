// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "metric.hpp"

#include "metricblob.pb.h"

#include "util.hpp"

#include <sys/statvfs.h>

#include <phosphor-logging/log.hpp>

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

namespace metric_blob
{

using phosphor::logging::entry;
using phosphor::logging::log;
using level = phosphor::logging::level;

BmcHealthSnapshot::BmcHealthSnapshot() :
    done(false), stringId(0), ticksPerSec(0)
{}

struct ProcStatEntry
{
    std::string cmdline;
    std::string tcomm;
    float utime;
    float stime;

    // Processes with the longest utime + stime are ranked first.
    // Tie breaking is done with cmdline then tcomm.
    bool operator<(const ProcStatEntry& other) const
    {
        const float negTime = -(utime + stime);
        const float negOtherTime = -(other.utime + other.stime);
        return std::tie(negTime, cmdline, tcomm) <
               std::tie(negOtherTime, other.cmdline, other.tcomm);
    }
};

bmcmetrics::metricproto::BmcProcStatMetric BmcHealthSnapshot::getProcStatList()
{
    constexpr std::string_view procPath = "/proc/";

    bmcmetrics::metricproto::BmcProcStatMetric ret;
    std::vector<ProcStatEntry> entries;

    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        int pid = -1;
        if (isNumericPath(path, pid))
        {
            ProcStatEntry entry;

            try
            {
                entry.cmdline = getCmdLine(pid);
                TcommUtimeStime t = getTcommUtimeStime(pid, ticksPerSec);
                entry.tcomm = t.tcomm;
                entry.utime = t.utime;
                entry.stime = t.stime;

                entries.push_back(entry);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>("Could not obtain process stats");
            }
        }
    }

    std::sort(entries.begin(), entries.end());

    bool isOthers = false;
    ProcStatEntry others;
    others.cmdline = "(Others)";
    others.utime = others.stime = 0;

    // Only show this many processes and aggregate all remaining ones into
    // "others" in order to keep the size of the snapshot reasonably small.
    // With 10 process stat entries and 10 FD count entries, the size of the
    // snapshot reaches around 1.5KiB. This is non-trivial, and we have to set
    // the collection interval long enough so as not to over-stress the IPMI
    // interface and the data collection service. The value of 10 is chosen
    // empirically, it might be subject to adjustments when the system is
    // launched later.
    constexpr int topN = 10;

    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (i >= topN)
        {
            isOthers = true;
        }

        ProcStatEntry& entry = entries[i];

        if (isOthers)
        {
            others.utime += entry.utime;
            others.stime += entry.stime;
        }
        else
        {
            bmcmetrics::metricproto::BmcProcStatMetric::BmcProcStat s;
            std::string fullCmdline = entry.cmdline;
            if (entry.tcomm.size() > 0)
            {
                fullCmdline += " " + entry.tcomm;
            }
            s.set_sidx_cmdline(getStringID(fullCmdline));
            s.set_utime(entry.utime);
            s.set_stime(entry.stime);
            *(ret.add_stats()) = s;
        }
    }

    if (isOthers)
    {
        bmcmetrics::metricproto::BmcProcStatMetric::BmcProcStat s;
        s.set_sidx_cmdline(getStringID(others.cmdline));
        s.set_utime(others.utime);
        s.set_stime(others.stime);
        *(ret.add_stats()) = s;
    }

    return ret;
}

int getFdCount(int pid)
{
    const std::string& fdPath = "/proc/" + std::to_string(pid) + "/fd";
    return std::distance(std::filesystem::directory_iterator(fdPath),
                         std::filesystem::directory_iterator{});
}

struct FdStatEntry
{
    int fdCount;
    std::string cmdline;
    std::string tcomm;

    // Processes with the largest fdCount goes first.
    // Tie-breaking using cmdline then tcomm.
    bool operator<(const FdStatEntry& other) const
    {
        const int negFdCount = -fdCount;
        const int negOtherFdCount = -other.fdCount;
        return std::tie(negFdCount, cmdline, tcomm) <
               std::tie(negOtherFdCount, other.cmdline, other.tcomm);
    }
};

bmcmetrics::metricproto::BmcFdStatMetric BmcHealthSnapshot::getFdStatList()
{
    bmcmetrics::metricproto::BmcFdStatMetric ret;

    // Sort by fd count, no tie-breaking
    std::vector<FdStatEntry> entries;

    const std::string_view procPath = "/proc/";
    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        int pid = 0;
        FdStatEntry entry;
        if (isNumericPath(path, pid))
        {
            try
            {
                entry.fdCount = getFdCount(pid);
                TcommUtimeStime t = getTcommUtimeStime(pid, ticksPerSec);
                entry.cmdline = getCmdLine(pid);
                entry.tcomm = t.tcomm;
                entries.push_back(entry);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>("Could not get file descriptor stats");
            }
        }
    }

    std::sort(entries.begin(), entries.end());

    bool isOthers = false;

    // Only report the detailed fd count and cmdline for the top 10 entries,
    // and collapse all others into "others".
    constexpr int topN = 10;

    FdStatEntry others;
    others.cmdline = "(Others)";
    others.fdCount = 0;

    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (i >= topN)
        {
            isOthers = true;
        }

        const FdStatEntry& entry = entries[i];
        if (isOthers)
        {
            others.fdCount += entry.fdCount;
        }
        else
        {
            bmcmetrics::metricproto::BmcFdStatMetric::BmcFdStat s;
            std::string fullCmdline = entry.cmdline;
            if (entry.tcomm.size() > 0)
            {
                fullCmdline += " " + entry.tcomm;
            }
            s.set_sidx_cmdline(getStringID(fullCmdline));
            s.set_fd_count(entry.fdCount);
            *(ret.add_stats()) = s;
        }
    }

    if (isOthers)
    {
        bmcmetrics::metricproto::BmcFdStatMetric::BmcFdStat s;
        s.set_sidx_cmdline(getStringID(others.cmdline));
        s.set_fd_count(others.fdCount);
        *(ret.add_stats()) = s;
    }

    return ret;
}

void BmcHealthSnapshot::serializeSnapshotToArray(
    const bmcmetrics::metricproto::BmcMetricSnapshot& snapshot)
{
    size_t size = snapshot.ByteSizeLong();
    if (size > 0)
    {
        pbDump.resize(size);
        if (!snapshot.SerializeToArray(pbDump.data(), size))
        {
            log<level::ERR>("Could not serialize protobuf to array");
        }
    }
}

void BmcHealthSnapshot::doWork()
{
    bmcmetrics::metricproto::BmcMetricSnapshot snapshot;

    // Memory info
    std::string meminfoBuffer = readFileThenGrepIntoString("/proc/meminfo");

    {
        bmcmetrics::metricproto::BmcMemoryMetric m;

        std::string_view sv(meminfoBuffer.data());
        // MemAvailable
        int value;
        bool ok = parseMeminfoValue(sv, "MemAvailable:", value);
        if (ok)
        {
            m.set_mem_available(value);
        }

        ok = parseMeminfoValue(sv, "Slab:", value);
        if (ok)
        {
            m.set_slab(value);
        }

        ok = parseMeminfoValue(sv, "KernelStack:", value);
        if (ok)
        {
            m.set_kernel_stack(value);
        }

        *(snapshot.mutable_memory_metric()) = m;
    }

    // Uptime
    std::string uptimeBuffer = readFileThenGrepIntoString("/proc/uptime");
    double uptime = 0;
    double idleProcessTime = 0;
    BootTimesMonotonic btm;
    if (!parseProcUptime(uptimeBuffer, uptime, idleProcessTime))
    {
        log<level::ERR>("Error parsing /proc/uptime");
    }
    else if (!getBootTimesMonotonic(btm))
    {
        log<level::ERR>("Could not get boot time");
    }
    else
    {
        bmcmetrics::metricproto::BmcUptimeMetric m1;
        m1.set_uptime(uptime);
        m1.set_idle_process_time(idleProcessTime);
        if (btm.firmwareTime == 0 && btm.powerOnSecCounterTime != 0)
        {
            m1.set_firmware_boot_time_sec(
                static_cast<double>(btm.powerOnSecCounterTime) - uptime);
        }
        else
        {
            m1.set_firmware_boot_time_sec(
                static_cast<double>(btm.firmwareTime - btm.loaderTime) / 1e6);
        }
        m1.set_loader_boot_time_sec(static_cast<double>(btm.loaderTime) / 1e6);
        // initrf presents
        if (btm.initrdTime != 0)
        {
            m1.set_kernel_boot_time_sec(static_cast<double>(btm.initrdTime) /
                                        1e6);
            m1.set_initrd_boot_time_sec(
                static_cast<double>(btm.userspaceTime - btm.initrdTime) / 1e6);
            m1.set_userspace_boot_time_sec(
                static_cast<double>(btm.finishTime - btm.userspaceTime) / 1e6);
        }
        else
        {
            m1.set_kernel_boot_time_sec(static_cast<double>(btm.userspaceTime) /
                                        1e6);
            m1.set_initrd_boot_time_sec(0);
            m1.set_userspace_boot_time_sec(
                static_cast<double>(btm.finishTime - btm.userspaceTime) / 1e6);
        }
        *(snapshot.mutable_uptime_metric()) = m1;
    }

    // Storage space
    struct statvfs fiData;
    if ((statvfs("/", &fiData)) < 0)
    {
        log<level::ERR>("Could not call statvfs");
    }
    else
    {
        uint64_t kib = (fiData.f_bsize * fiData.f_bfree) / 1024;
        bmcmetrics::metricproto::BmcDiskSpaceMetric m2;
        m2.set_rwfs_kib_available(static_cast<int>(kib));
        *(snapshot.mutable_storage_space_metric()) = m2;
    }

    // The next metrics require a sane ticks_per_sec value, typically 100 on
    // the BMC. In the very rare circumstance when it's 0, exit early and return
    // a partially complete snapshot (no process).
    ticksPerSec = getTicksPerSec();

    // FD stat
    *(snapshot.mutable_fdstat_metric()) = getFdStatList();

    if (ticksPerSec == 0)
    {
        log<level::ERR>("ticksPerSec is 0, skipping the process list metric");
        serializeSnapshotToArray(snapshot);
        done = true;
        return;
    }

    // Proc stat
    *(snapshot.mutable_procstat_metric()) = getProcStatList();

    // String table
    std::vector<std::string_view> strings(stringTable.size());
    for (const auto& [s, i] : stringTable)
    {
        strings[i] = s;
    }

    bmcmetrics::metricproto::BmcStringTable st;
    for (size_t i = 0; i < strings.size(); ++i)
    {
        bmcmetrics::metricproto::BmcStringTable::StringEntry entry;
        entry.set_value(strings[i].data());
        *(st.add_entries()) = entry;
    }
    *(snapshot.mutable_string_table()) = st;

    // Save to buffer
    serializeSnapshotToArray(snapshot);
    done = true;
}

// BmcBlobSessionStat (9) but passing meta as reference instead of pointer,
// since the metadata must not be null at this point.
bool BmcHealthSnapshot::stat(blobs::BlobMeta& meta)
{
    if (!done)
    {
        // Bits 8~15 are blob-specific state flags.
        // For this blob, bit 8 is set when metric collection is still in
        // progress.
        meta.blobState |= (1 << 8);
    }
    else
    {
        meta.blobState = 0;
        meta.blobState = blobs::StateFlags::open_read;
        meta.size = pbDump.size();
    }
    return true;
}

std::string_view BmcHealthSnapshot::read(uint32_t offset,
                                         uint32_t requestedSize)
{
    uint32_t size = static_cast<uint32_t>(pbDump.size());
    if (offset >= size)
    {
        return {};
    }
    return std::string_view(pbDump.data() + offset,
                            std::min(requestedSize, size - offset));
}

int BmcHealthSnapshot::getStringID(const std::string_view s)
{
    int ret = 0;
    auto itr = stringTable.find(s.data());
    if (itr == stringTable.end())
    {
        stringTable[s.data()] = stringId;
        ret = stringId;
        ++stringId;
    }
    else
    {
        ret = itr->second;
    }
    return ret;
}

} // namespace metric_blob
