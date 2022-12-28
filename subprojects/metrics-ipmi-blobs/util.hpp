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

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace metric_blob
{

struct ProcStatInfo
{
    std::string tcomm;
    float utime;
    float stime;
    uint32_t rss;
    float uptime;
    float cpu_util;
};

struct BootTimesMonotonic
{
    uint64_t firmwareTime = 0;
    uint64_t loaderTime = 0;
    uint64_t initrdTime = 0;
    uint64_t userspaceTime = 0;
    uint64_t finishTime = 0;
    uint64_t powerOnSecCounterTime = 0;
};

ProcStatInfo parseProcessStatsString(std::string_view content,
                                     long ticksPerSec);
std::string readFileThenGrepIntoString(std::string_view fileName,
                                       std::string_view grepStr = "");
bool isNumericPath(std::string_view path, int& value);
ProcStatInfo getProcessStats(int pid, long ticksPerSec);
std::string getCmdLine(int pid);
bool parseMeminfoValue(std::string_view content, std::string_view keyword,
                       int& value);
bool parseProcUptime(const std::string_view content, double& uptime,
                     double& idleProcessTime);
bool readMem(const uint32_t target, uint32_t& memResult);
bool getBootTimesMonotonic(BootTimesMonotonic& btm);
long getTicksPerSec();
char controlCharsToSpace(char c);
std::string trimStringRight(std::string_view s);

std::optional<std::string> getDaemonObjectPathFromPid(const int pid);
std::string getDaemonNameFromObjectPath(const std::string daemonObjectPath);
int32_t getDaemonRestartCountFromObjectPath(const std::string daemonObjectPath);
float getDaemonUptimeFromPid(const int pid, const long ticksPerSec);

} // namespace metric_blob
