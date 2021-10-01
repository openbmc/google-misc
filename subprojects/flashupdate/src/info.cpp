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

#include <fmt/format.h>

#include <flashupdate/args.hpp>
#include <flashupdate/info.hpp>
#include <flashupdate/logging.hpp>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace flashupdate
{
namespace info
{

std::string hashToString(const std::vector<uint8_t>& hash)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < 32; ++i)
    {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }

    return ss.str();
}

void print(std::string_view prefix, std::string_view message, bool clean)
{
    if (clean)
    {
        fmt::print(stdout, message);
    }
    else
    {
        log(LogLevel::Notice, "{}: {}\n", prefix, message);
    }
}

std::string listStates()
{
    std::string list;
    for (const auto& [state, _] : info::stringToState)
    {
        list += fmt::format("  `{}`\n", state);
    }

    return list;
}

void printStatus(const Args& args, struct Status status)
{

    if (args.checkStageVersion)
    {
        print("Stage Version",
              fmt::format("{}.{}.{}.0", status.stage.major, status.stage.minor,
                          status.stage.point),
              args.cleanOutput);
    }

    if (args.checkActiveVersion)
    {
        print("Active Version",
              fmt::format("{}.{}.{}.0", status.active.major,
                          status.active.minor, status.active.point),
              args.cleanOutput);
    }

    if (args.checkStageState)
    {
        auto findState = stateToString.find(status.state);
        std::string_view state = "CORRUPTED";
        if (findState != stateToString.end())
        {
            state = findState->second;
        }
        print("Status Staged State", state, args.cleanOutput);
    }

    if (args.otherInfo)
    {
        print("Staging Index", std::to_string(status.stagingIndex),
              args.cleanOutput);

        print("CR51 Descriptor Hash",
              hashToString(std::vector<uint8_t>(&status.descriptorHash[0],
                                                &status.descriptorHash[0] +
                                                    SHA256_DIGEST_LENGTH)),
              args.cleanOutput);
    }
}
} // namespace info
} // namespace flashupdate
