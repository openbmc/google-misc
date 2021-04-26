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

#include "util.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace metric_blob
{

using phosphor::logging::log;
using level = phosphor::logging::level;

char controlCharsToSpace(char c)
{
    if (c < 32)
    {
        c = ' ';
    }
    return c;
}

long getTicksPerSec()
{
    return sysconf(_SC_CLK_TCK);
}

std::string readFileThenGrepIntoString(const std::string_view fileName,
                                       const std::string_view grepStr)
{
    std::stringstream ss;
    std::ifstream ifs(fileName.data());
    while (ifs.good())
    {
        std::string line;
        std::getline(ifs, line);
        if (line.find(grepStr) != std::string::npos)
        {
            ss << line;
        }
        if (ifs.good())
            ss << std::endl;
    }
    return ss.str();
}

bool isNumericPath(const std::string_view path, int& value)
{
    size_t p = path.rfind('/');
    if (p == std::string::npos)
    {
        return false;
    }
    int id = 0;
    for (size_t i = p + 1; i < path.size(); ++i)
    {
        const char ch = path[i];
        if (ch < '0' || ch > '9')
            return false;
        else
        {
            id = id * 10 + (ch - '0');
        }
    }
    value = id;
    return true;
}

// Trims all control characters at the end of a string.
std::string trimStringRight(std::string_view s)
{
    std::string ret(s.data());
    while (!ret.empty())
    {
        if (ret.back() <= 32)
            ret.pop_back();
        else
            break;
    }
    return ret;
}

std::string getCmdLine(const int pid)
{
    const std::string& cmdlinePath =
        "/proc/" + std::to_string(pid) + "/cmdline";

    std::string cmdline = readFileThenGrepIntoString(cmdlinePath);
    for (size_t i = 0; i < cmdline.size(); ++i)
    {
        cmdline[i] = controlCharsToSpace(cmdline[i]);
    }

    // Trim empty strings
    cmdline = trimStringRight(cmdline);

    return cmdline;
}

// strtok is used in this function in order to avoid usage of <sstream>.
// However, that would require us to create a temporary std::string.
TcommUtimeStime parseTcommUtimeStimeString(std::string_view content,
                                           const long ticksPerSec)
{
    TcommUtimeStime ret;
    ret.tcomm = "";
    ret.utime = ret.stime = 0;

    const float invTicksPerSec = 1.0f / static_cast<float>(ticksPerSec);

    // pCol now points to the first part in content after content is split by
    // space.
    // This is not ideal,
    std::string temp(content);
    char* pCol = strtok(temp.data(), " ");

    if (pCol != nullptr)
    {
        const int fields[] = {1, 13, 14}; // tcomm, utime, stime
        int fieldIdx = 0;
        for (int colIdx = 0; colIdx < 15; ++colIdx)
        {
            if (fieldIdx < 3 && colIdx == fields[fieldIdx])
            {
                switch (fieldIdx)
                {
                    case 0:
                    {
                        ret.tcomm = std::string(pCol);
                        break;
                    }
                    case 1:
                        [[fallthrough]];
                    case 2:
                    {
                        int ticks = std::atoi(pCol);
                        float t = static_cast<float>(ticks) * invTicksPerSec;

                        if (fieldIdx == 1)
                        {
                            ret.utime = t;
                        }
                        else if (fieldIdx == 2)
                        {
                            ret.stime = t;
                        }
                        break;
                    }
                }
                ++fieldIdx;
            }
            pCol = strtok(nullptr, " ");
        }
    }

    if (ticksPerSec <= 0)
    {
        log<level::ERR>("ticksPerSec is equal or less than zero");
    }

    return ret;
}

TcommUtimeStime getTcommUtimeStime(const int pid, const long ticksPerSec)
{
    const std::string& statPath = "/proc/" + std::to_string(pid) + "/stat";
    return parseTcommUtimeStimeString(readFileThenGrepIntoString(statPath),
                                      ticksPerSec);
}

// Returns true if successfully parsed and false otherwise. If parsing was
// successful, value is set accordingly.
// Input: "MemAvailable:      1234 kB"
// Returns true, value set to 1234
bool parseMeminfoValue(const std::string_view content,
                       const std::string_view keyword, int& value)
{
    size_t p = content.find(keyword);
    if (p != std::string::npos)
    {
        std::string_view v = content.substr(p + keyword.size());
        p = v.find("kB");
        if (p != std::string::npos)
        {
            v = v.substr(0, p);
            value = std::atoi(v.data());
            return true;
        }
    }
    return false;
}

bool parseProcUptime(const std::string_view content, double& uptime,
                     double& idleProcessTime)
{
    double t0, t1; // Attempts to parse uptime and idleProcessTime
    int ret = sscanf(content.data(), "%lf %lf", &t0, &t1);
    if (ret == 2 && std::isfinite(t0) && std::isfinite(t1))
    {
        uptime = t0;
        idleProcessTime = t1;
        return true;
    }
    return false;
}

bool readMem(const uint32_t target, uint32_t& memResult)
{
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0)
    {
        return false;
    }

    int pageSize = getpagesize();
    uint32_t pageOffset = target & ~static_cast<uint32_t>(pageSize - 1);
    uint32_t offsetInPage = target & static_cast<uint32_t>(pageSize - 1);

    void* mapBase =
        mmap(NULL, pageSize * 2, PROT_READ, MAP_SHARED, fd, pageOffset);
    if (mapBase == MAP_FAILED)
    {
        close(fd);
        return false;
    }

    char* virtAddr = reinterpret_cast<char*>(mapBase) + offsetInPage;
    memResult = *(reinterpret_cast<uint32_t*>(virtAddr));
    close(fd);
    return true;
}

// clang-format off
/*
 *  power-on
 *  counter(start)                 uptime(start)
 *  firmware(Neg)    loader(Neg)   kernel(always 0)    initrd                 userspace              finish
 *  |----------------|-------------|-------------------|----------------------|----------------------|
 *  |----------------| <--- firmwareTime=firmware-loader
 *                   |-------------| <--- loaderTime=loader
 *  |------------------------------| <--- firmwareTime(Actually is firmware+loader)=counter-uptime \
 *                                        (in this case we can treat this as firmware time \
 *                                         since firmware consumes most of the time)
 *                                 |-------------------| <--- kernelTime=initrd (if initrd present)
 *                                 |------------------------------------------| <--- kernelTime=userspace (if no initrd)
 *                                                     |----------------------| <--- initrdTime=userspace-initrd (if initrd present)
 *                                                                            |----------------------| <--- userspaceTime=finish-userspace
 */
// clang-format on
bool getBootTimesMonotonic(BootTimesMonotonic& btm)
{
    // Timestamp name and its offset in the struct.
    std::vector<std::pair<std::string_view, size_t>> timeMap = {
        {"FirmwareTimestampMonotonic",
         offsetof(BootTimesMonotonic, firmwareTime)}, // negative value
        {"LoaderTimestampMonotonic",
         offsetof(BootTimesMonotonic, loaderTime)}, // negative value
        {"InitRDTimestampMonotonic", offsetof(BootTimesMonotonic, initrdTime)},
        {"UserspaceTimestampMonotonic",
         offsetof(BootTimesMonotonic, userspaceTime)},
        {"FinishTimestampMonotonic", offsetof(BootTimesMonotonic, finishTime)}};

    auto b = sdbusplus::bus::new_default_system();
    auto m = b.new_method_call("org.freedesktop.systemd1",
                               "/org/freedesktop/systemd1",
                               "org.freedesktop.DBus.Properties", "GetAll");
    m.append("");
    auto reply = b.call(m);
    std::vector<std::pair<std::string, std::variant<uint64_t>>> timestamps;
    reply.read(timestamps);

    // Parse timestamps from dbus result.
    auto btmPtr = reinterpret_cast<char*>(&btm);
    unsigned int recordCnt = 0;
    for (auto& t : timestamps)
    {
        for (auto& tm : timeMap)
        {
            if (tm.first.compare(t.first) == 0)
            {
                auto temp = std::get<uint64_t>(t.second);
                memcpy(btmPtr + tm.second, reinterpret_cast<char*>(&temp),
                       sizeof(temp));
                recordCnt++;
                break;
            }
        }
        if (recordCnt == timeMap.size())
        {
            break;
        }
    }
    if (recordCnt != timeMap.size())
    {
        log<level::ERR>("Didn't get desired timestamps");
        return false;
    }

    std::string cpuinfo =
        readFileThenGrepIntoString("/proc/cpuinfo", "Hardware");
    // Nuvoton NPCM7XX chip has a counter which starts from power-on.
    if (cpuinfo.find("NPCM7XX") != std::string::npos)
    {
        // Get elapsed seconds from SEC_CNT register
        const uint32_t SEC_CNT_ADDR = 0xf0801068;
        uint32_t memResult = 0;
        if (readMem(SEC_CNT_ADDR, memResult))
        {
            btm.powerOnSecCounterTime = static_cast<uint64_t>(memResult);
        }
        else
        {
            log<level::ERR>("Read memory SEC_CNT_ADDR(0xf0801068) failed");
            return false;
        }
    }

    return true;
}

} // namespace metric_blob