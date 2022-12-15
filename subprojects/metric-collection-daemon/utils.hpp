#pragma once

#include <sdbusplus/message.hpp>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using Association = std::tuple<std::string, std::string, std::string>;
using ExecStartType = std::vector<
    std::tuple<std::string, std::vector<std::string>, bool, uint64_t, uint64_t,
               uint64_t, uint64_t, uint32_t, int32_t, int32_t>>;
using ExecStartVariantType = std::variant<ExecStartType>;

using DbusVariantType = std::variant<uint32_t, uint64_t, std::string,
                                     sdbusplus::message::object_path>;
// a(sasbttttuii)

using DBusPropertiesMap = std::vector<std::pair<std::string, DbusVariantType>>;

std::optional<int> isNumericPath(std::string_view path);
long getTicksPerSec();
std::string_view readFileIntoString(const std::string_view fileName);
std::vector<std::string> split(std::string_view input, char delim);

size_t toSizeT(std::string intString);

std::array<size_t, 3> parseBootInfo();

std::optional<std::string> getPortIdByNum(const int portNum);