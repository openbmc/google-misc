#pragma once

#include <string>
#include <string_view>

namespace metric_blob
{

struct TcommUtimeStime
{
    std::string tcomm;
    float utime;
    float stime;
};

TcommUtimeStime parseTcommUtimeStimeString(std::string_view content,
                                           long ticksPerSec);
std::string readFileIntoString(std::string_view fileName);
bool isNumericPath(std::string_view path, int& value);
TcommUtimeStime getTcommUtimeStime(int pid, long ticksPerSec);
std::string getCmdLine(int pid);
bool parseMeminfoValue(std::string_view content, std::string_view keyword,
                       int& value);
bool parseProcUptime(const std::string_view content, double& uptime,
                     double& idleProcessTime);
long getTicksPerSec();
char controlCharsToSpace(char c);
std::string trimStringRight(std::string_view s);

} // namespace metric_blob
