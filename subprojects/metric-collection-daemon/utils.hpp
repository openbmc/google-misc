#pragma once

#include <string.h>

#include <sdbusplus/message.hpp>

#include <array>
#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using DbusVariantType =
    std::variant<uint32_t, uint64_t, std::string, sdbusplus::message::object_path>;
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
        {
            ss << std::endl;
        }
    }
    ifs.close();
    return ss.str();
};

std::array<uint64_t, 3> parseBootInfo()
{
    const std::string& bootInfoPath = "/var/google/bootinfo";
    std::string bootinfoFile = readFileIntoString(bootInfoPath);
    char* col = strtok(bootinfoFile.data(), " ");

    // {Boot Count, Crash Count}
    std::array<uint64_t, 3> bootinfo{0, 0, 0};

    // If file does not exist, then just set boot and crash counts to 0

    for (size_t i = 0; i < bootinfo.size(); ++i)
    {
        if (col == NULL)
        {
            break;
        }

        bootinfo[i] = static_cast<uint64_t>(std::stoull(col));
        col = strtok(NULL, " ");
    }

    return bootinfo;
};