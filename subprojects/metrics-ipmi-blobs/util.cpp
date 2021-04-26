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

std::string readFileIntoString(const std::string_view fileName)
{
    std::stringstream ss;
    std::ifstream ifs(fileName.data());
    while (ifs.good())
    {
        std::string line;
        std::getline(ifs, line);
        ss << line;
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

    std::string cmdline = readFileIntoString(cmdlinePath);
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
    return parseTcommUtimeStimeString(readFileIntoString(statPath),
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
    void *mapBase, *virtAddr;
    unsigned pageSize, mappedSize, offsetInPage;
    int fd;
    unsigned width = 8 * sizeof(int);

    fd = open("/dev/mem", O_RDONLY | O_SYNC);
    mappedSize = pageSize = getpagesize();
    offsetInPage = (unsigned)target & (pageSize - 1);
    if (offsetInPage + width > pageSize)
    {
        /* This access spans pages.
         * Must map two pages to make it possible: */
        mappedSize *= 2;
    }
    mapBase = mmap(NULL, mappedSize, PROT_READ, MAP_SHARED, fd,
                   target & ~(off_t)(pageSize - 1));
    if (mapBase == MAP_FAILED)
    {
        return false;
    }
    virtAddr = (char*)mapBase + offsetInPage;
    memResult = *(volatile uint32_t*)virtAddr;
    return true;
}

bool getBoottime(double& powerOnCounterTime, double& kernelTime,
                 double& systemdTime)
{
    // kernel time and systemd time are the only 2 timestamp we can obtain.
    std::unordered_map<std::string_view, uint64_t> timeMap = {
        {"UserspaceTimestampMonotonic", 0},
        {"FinishTimestampMonotonic", 0},
    };

    auto b = sdbusplus::bus::new_default_system();
    auto m = b.new_method_call("org.freedesktop.systemd1",
                               "/org/freedesktop/systemd1",
                               "org.freedesktop.DBus.Properties", "GetAll");
    m.append("");
    auto reply = b.call(m);
    std::vector<std::pair<std::string, std::variant<uint64_t>>> timestamps;
    reply.read(timestamps);

    // Parse kernel timestamp and systemd timestamp from dbus result.
    unsigned int recordCnt = 0;
    for (auto& t : timestamps)
    {
        for (auto& tm : timeMap)
        {
            if (tm.first.compare(t.first) == 0)
            {
                tm.second = std::get<uint64_t>(t.second);
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
        // Didn't get desired timestamp.
        return false;
    }

    // Get elapsed seconds from SEC_CNT register
    const uint32_t SEC_CNT_ADDR = 0xf0801068;
    uint32_t memResult = 0;
    if (!readMem(SEC_CNT_ADDR, memResult))
    {
        return false;
    }

    powerOnCounterTime = memResult;
    kernelTime = timeMap["UserspaceTimestampMonotonic"];
    systemdTime = timeMap["FinishTimestampMonotonic"] - kernelTime;

    return true;
}

} // namespace metric_blob