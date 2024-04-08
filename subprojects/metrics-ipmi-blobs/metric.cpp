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

#include "metricblob.pb.n.h"

#include "util.hpp"

#include <pb_encode.h>
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

template <typename T>
static constexpr auto pbEncodeStr = [](pb_ostream_t* stream,
                                       const pb_field_iter_t* field,
                                       void* const* arg) noexcept {
    static_assert(sizeof(*std::declval<T>().data()) == sizeof(pb_byte_t));
    const auto& s = *reinterpret_cast<const T*>(*arg);
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(
               stream, reinterpret_cast<const pb_byte_t*>(s.data()), s.size());
};

template <typename T>
static pb_callback_t pbStrEncoder(const T& t) noexcept
{
    return {{.encode = pbEncodeStr<T>}, const_cast<T*>(&t)};
}

template <auto fields, typename T>
static constexpr auto pbEncodeSubs = [](pb_ostream_t* stream,
                                        const pb_field_iter_t* field,
                                        void* const* arg) noexcept {
    for (const auto& sub : *reinterpret_cast<const std::vector<T>*>(*arg))
    {
        if (!pb_encode_tag_for_field(stream, field) ||
            !pb_encode_submessage(stream, fields, &sub))
        {
            return false;
        }
    }
    return true;
};

template <auto fields, typename T>
static pb_callback_t pbSubsEncoder(const std::vector<T>& t)
{
    return {{.encode = pbEncodeSubs<fields, T>},
            const_cast<std::vector<T>*>(&t)};
}

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

static bmcmetrics_metricproto_BmcProcStatMetric getProcStatMetric(
    BmcHealthSnapshot& obj, long ticksPerSec,
    std::vector<bmcmetrics_metricproto_BmcProcStatMetric_BmcProcStat>& procs,
    bool& use) noexcept
{
    if (ticksPerSec == 0)
    {
        return {};
    }
    constexpr std::string_view procPath = "/proc/";

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

        const ProcStatEntry& entry = entries[i];

        if (isOthers)
        {
            others.utime += entry.utime;
            others.stime += entry.stime;
        }
        else
        {
            std::string fullCmdline = entry.cmdline;
            if (entry.tcomm.size() > 0)
            {
                fullCmdline += " ";
                fullCmdline += entry.tcomm;
            }
            procs.emplace_back(
                bmcmetrics_metricproto_BmcProcStatMetric_BmcProcStat{
                    .sidx_cmdline = obj.getStringID(fullCmdline),
                    .utime = entry.utime,
                    .stime = entry.stime,
                });
        }
    }

    if (isOthers)
    {
        procs.emplace_back(bmcmetrics_metricproto_BmcProcStatMetric_BmcProcStat{
            .sidx_cmdline = obj.getStringID(others.cmdline),
            .utime = others.utime,
            .stime = others.stime,

        });
    }

    use = true;
    return bmcmetrics_metricproto_BmcProcStatMetric{
        .stats = pbSubsEncoder<
            bmcmetrics_metricproto_BmcProcStatMetric_BmcProcStat_fields>(procs),
    };
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

static bmcmetrics_metricproto_BmcFdStatMetric getFdStatMetric(
    BmcHealthSnapshot& obj, long ticksPerSec,
    std::vector<bmcmetrics_metricproto_BmcFdStatMetric_BmcFdStat>& fds,
    bool& use) noexcept
{
    if (ticksPerSec == 0)
    {
        return {};
    }

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
            std::string fullCmdline = entry.cmdline;
            if (entry.tcomm.size() > 0)
            {
                fullCmdline += " ";
                fullCmdline += entry.tcomm;
            }
            fds.emplace_back(bmcmetrics_metricproto_BmcFdStatMetric_BmcFdStat{
                .sidx_cmdline = obj.getStringID(fullCmdline),
                .fd_count = entry.fdCount,
            });
        }
    }

    if (isOthers)
    {
        fds.emplace_back(bmcmetrics_metricproto_BmcFdStatMetric_BmcFdStat{
            .sidx_cmdline = obj.getStringID(others.cmdline),
            .fd_count = others.fdCount,
        });
    }

    use = true;
    return bmcmetrics_metricproto_BmcFdStatMetric{
        .stats = pbSubsEncoder<
            bmcmetrics_metricproto_BmcFdStatMetric_BmcFdStat_fields>(fds),
    };
}

static bmcmetrics_metricproto_BmcECCMetric getECCMetric(bool& use) noexcept
{
    std::optional<bmcmetrics_metricproto_BmcECCMetric> metric =
        getECCErrorCounts();
    use = metric.has_value();
    if (use)
    {
        return *metric;
    }
    return {};
}

static bmcmetrics_metricproto_BmcMemoryMetric getMemMetric() noexcept
{
    bmcmetrics_metricproto_BmcMemoryMetric ret = {};
    auto data = readFileThenGrepIntoString("/proc/meminfo");
    int value;
    if (parseMeminfoValue(data, "MemAvailable:", value))
    {
        ret.mem_available = value;
    }
    if (parseMeminfoValue(data, "Slab:", value))
    {
        ret.slab = value;
    }

    if (parseMeminfoValue(data, "KernelStack:", value))
    {
        ret.kernel_stack = value;
    }
    return ret;
}

static bmcmetrics_metricproto_BmcUptimeMetric
    getUptimeMetric(bool& use) noexcept
{
    bmcmetrics_metricproto_BmcUptimeMetric ret = {};

    double uptime = 0;
    {
        auto data = readFileThenGrepIntoString("/proc/uptime");
        double idleProcessTime = 0;
        if (!parseProcUptime(data, uptime, idleProcessTime))
        {
            log<level::ERR>("Error parsing /proc/uptime");
            return ret;
        }
        ret.uptime = uptime;
        ret.idle_process_time = idleProcessTime;
    }

    BootTimesMonotonic btm;
    if (!getBootTimesMonotonic(btm))
    {
        log<level::ERR>("Could not get boot time");
        return ret;
    }
    if (btm.firmwareTime == 0 && btm.powerOnSecCounterTime != 0)
    {
        ret.firmware_boot_time_sec =
            static_cast<double>(btm.powerOnSecCounterTime) - uptime;
    }
    else
    {
        ret.firmware_boot_time_sec =
            static_cast<double>(btm.firmwareTime - btm.loaderTime) / 1e6;
    }
    ret.loader_boot_time_sec = static_cast<double>(btm.loaderTime) / 1e6;
    if (btm.initrdTime != 0)
    {
        ret.kernel_boot_time_sec = static_cast<double>(btm.initrdTime) / 1e6;
        ret.initrd_boot_time_sec =
            static_cast<double>(btm.userspaceTime - btm.initrdTime) / 1e6;
        ret.userspace_boot_time_sec =
            static_cast<double>(btm.finishTime - btm.userspaceTime) / 1e6;
    }
    else
    {
        ret.kernel_boot_time_sec = static_cast<double>(btm.userspaceTime) / 1e6;
        ret.initrd_boot_time_sec = 0;
        ret.userspace_boot_time_sec =
            static_cast<double>(btm.finishTime - btm.userspaceTime) / 1e6;
    }

    use = true;
    return ret;
}

static bmcmetrics_metricproto_BmcDiskSpaceMetric
    getStorageMetric(bool& use) noexcept
{
    bmcmetrics_metricproto_BmcDiskSpaceMetric ret = {};
    struct statvfs fiData;
    if (statvfs("/", &fiData) < 0)
    {
        log<level::ERR>("Could not call statvfs");
    }
    else
    {
        ret.rwfs_kib_available = (fiData.f_bsize * fiData.f_bfree) / 1024;
        use = true;
    }
    return ret;
}

void BmcHealthSnapshot::doWork()
{
    // The next metrics require a sane ticks_per_sec value, typically 100 on
    // the BMC. In the very rare circumstance when it's 0, exit early and return
    // a partially complete snapshot (no process).
    ticksPerSec = getTicksPerSec();

    static constexpr auto stcb = [](pb_ostream_t* stream,
                                    const pb_field_t* field,
                                    void* const* arg) noexcept {
        auto& self = *reinterpret_cast<BmcHealthSnapshot*>(*arg);
        std::vector<std::string_view> strs(self.stringTable.size());
        for (const auto& [str, i] : self.stringTable)
        {
            strs[i] = str;
        }
        for (auto& str : strs)
        {
            bmcmetrics_metricproto_BmcStringTable_StringEntry msg = {
                .value = pbStrEncoder(str),
            };
            if (!pb_encode_tag_for_field(stream, field) ||
                !pb_encode_submessage(
                    stream,
                    bmcmetrics_metricproto_BmcStringTable_StringEntry_fields,
                    &msg))
            {
                return false;
            }
        }
        return true;
    };
    std::vector<bmcmetrics_metricproto_BmcProcStatMetric_BmcProcStat> procs;
    std::vector<bmcmetrics_metricproto_BmcFdStatMetric_BmcFdStat> fds;
    bmcmetrics_metricproto_BmcMetricSnapshot snapshot = {
        .has_string_table = true,
        .string_table =
            {
                .entries = {{.encode = stcb}, this},
            },
        .has_memory_metric = true,
        .memory_metric = getMemMetric(),
        .has_uptime_metric = false,
        .uptime_metric = getUptimeMetric(snapshot.has_uptime_metric),
        .has_storage_space_metric = false,
        .storage_space_metric =
            getStorageMetric(snapshot.has_storage_space_metric),
        .has_procstat_metric = false,
        .procstat_metric = getProcStatMetric(*this, ticksPerSec, procs,
                                             snapshot.has_procstat_metric),
        .has_fdstat_metric = false,
        .fdstat_metric = getFdStatMetric(*this, ticksPerSec, fds,
                                         snapshot.has_fdstat_metric),
        .has_ecc_metric = false,
        .ecc_metric = getECCMetric(snapshot.has_ecc_metric),
    };
    pb_ostream_t nost = {};
    if (!pb_encode(&nost, bmcmetrics_metricproto_BmcMetricSnapshot_fields,
                   &snapshot))
    {
        auto msg = std::format("Getting pb size: {}", PB_GET_ERROR(&nost));
        log<level::ERR>(msg.c_str());
        return;
    }
    pbDump.resize(nost.bytes_written);
    auto ost = pb_ostream_from_buffer(
        reinterpret_cast<pb_byte_t*>(pbDump.data()), pbDump.size());
    if (!pb_encode(&ost, bmcmetrics_metricproto_BmcMetricSnapshot_fields,
                   &snapshot))
    {
        auto msg = std::format("Writing pb msg: {}", PB_GET_ERROR(&ost));
        log<level::ERR>(msg.c_str());
        return;
    }
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
