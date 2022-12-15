#include "utils.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include <sdbusplus/message.hpp>

#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using DbusVariantType = std::variant<uint32_t, uint64_t, std::string,
                                     sdbusplus::message::object_path>;
using Association = std::tuple<std::string, std::string, std::string>;

using DBusPropertiesMap = std::vector<std::pair<std::string, DbusVariantType>>;

std::optional<int> isNumericPath(std::string_view path)
{
    size_t p = path.rfind('/');
    if (p == std::string::npos)
    {
        return std::nullopt;
    }
    int id = 0;
    std::string_view pid = path.substr(p + 1);

    auto res = std::from_chars(pid.data(), pid.data() + pid.size(), id);
    if (res.ec != std::errc())
    {
        return std::nullopt;
    }

    return id;
}

long getTicksPerSec()
{
    return sysconf(_SC_CLK_TCK);
}

std::string_view readFileIntoString(const std::string_view fileName)
{
    std::stringstream ss;
    std::ifstream ifs(fileName.data());
    while (ifs.good())
    {
        std::string line;
        std::getline(ifs, line);
        ss << line;
        if (ifs.good())
        {
            ss << std::endl;
        }
    }
    ifs.close();
    return ss.str();
};

std::vector<std::string> split(std::string_view input, char delim)
{
    std::vector<std::string> ret;
    for (size_t start = 0, pos = 0; pos != std::string_view::npos; start = pos)
    {
        pos = input.find_first_of(delim, start);
        std::string_view token = input.substr(start, pos - start);
        if (!token.empty())
        {
            ret.emplace_back(std::string(token));
        }

        pos = input.find_first_not_of(delim, pos);
        if (pos == std::string_view::npos)
        {
            break;
        }
    }
    return ret;
}

size_t toSizeT(std::string intString)
{
    size_t num;
    auto res = std::from_chars(intString.data(),
                               intString.data() + intString.size(), num);

    if (res.ec != std::errc())
    {
        num = 0;
    }

    return num;
}

size_t toSizeT(std::string_view intString)
{
    size_t num;
    auto res = std::from_chars(intString.data(),
                               intString.data() + intString.size(), num);

    if (res.ec != std::errc())
    {
        num = 0;
    }

    return num;
}

std::array<size_t, 3> parseBootInfo()
{
    const std::string& bootInfoPath = "/var/google/bootinfo";
    std::string_view bootinfoFile = readFileIntoString(bootInfoPath);
    std::array<size_t, 3> bootinfo{0, 0, 0};

    // If file does not exist, then just set boot and crash counts to 0
    if (bootinfoFile.empty())
    {
        return bootinfo;
    }

    std::vector<std::string> bootinfoSplit = split(bootinfoFile, ' ');

    // {Boot Count, Crash Count, Last Boot Update}

    for (uint i = 0; i < bootinfoSplit.size(); ++i)
    {
        bootinfo[i] = toSizeT(bootinfoSplit[i]);
    }

    return bootinfo;
};
