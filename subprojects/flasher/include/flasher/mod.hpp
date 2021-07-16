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
#include <fmt/format.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace flasher
{

struct ModArgs
{
    std::vector<std::string> arr;
    std::unordered_map<std::string, std::string> dict;

    ModArgs(std::string_view str);

    inline bool operator==(const ModArgs& other) const
    {
        return arr == other.arr && dict == other.dict;
    }
};

template <typename Mod>
class ModType
{
  public:
    virtual ~ModType() = default;
    virtual void printHelp() const = 0;
};

template <typename ModType>
using ModTypeMap =
    std::unordered_map<std::string_view, std::unique_ptr<ModType>>;

template <typename Mod, typename Type = ModType<Mod>, typename... Ts>
std::unique_ptr<Mod> openMod(const ModTypeMap<Type>& map, const ModArgs& args,
                             Ts&&... ts)
{
    if (args.arr.empty())
    {
        throw std::invalid_argument("Missing type");
    }
    auto it = map.find(args.arr[0]);
    if (it == map.end())
    {
        throw std::invalid_argument(
            fmt::format("Unknown type: {}", args.arr[0]));
    }
    return it->second->open(args, std::forward<Ts>(ts)...);
}

} // namespace flasher
