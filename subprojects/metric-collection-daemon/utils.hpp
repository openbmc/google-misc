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

using DbusVariantType = std::variant<uint32_t, uint64_t, std::string,
                                     sdbusplus::message::object_path>;
using Association = std::tuple<std::string, std::string, std::string>;

using DBusPropertiesMap = std::vector<std::pair<std::string, DbusVariantType>>;

std::optional<int> isNumericPath(std::string_view path);
long getTicksPerSec();
std::string_view readFileIntoString(const std::string_view fileName);
std::vector<std::string> split(std::string_view input, char delim);

size_t toSizeT(std::string intString);
size_t toSizeT(std::string_view intString);

std::array<size_t, 3> parseBootInfo();
