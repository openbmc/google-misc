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

/** @brief Helper to print debug message or std output
 *
 * @param[in] prefix    Debug message prefix
 * @param[in] message   Main message
 * @param[in] output    Print the message as std output with no debug message
 *
 * @return string copy of the message
 */
static std::string print(std::string_view prefix, std::string_view message,
                         bool output)
{
    if (output)
    {
        fmt::print(stdout, message);
    }
    else
    {
        message = fmt::format("{}: {}\n", prefix, message);
        log(LogLevel::Notice, message);
    }
    return message.data();
}

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

std::string listStates()
{
    std::string list;
    for (const auto& [state, _] : info::stringToState)
    {
        list += fmt::format("  `{}`\n", state);
    }

    return list;
}

std::string printUpdateInfo(const Args& args, struct UpdateInfo info)
{
    std::string message;
    if (args.checkStageVersion)
    {
        message += print("Stage Version",
                         fmt::format("{}.{}.{}.0", info.stage.major,
                                     info.stage.minor, info.stage.point),
                         args.cleanOutput);
    }

    if (args.checkActiveVersion)
    {
        message += print("Active Version",
                         fmt::format("{}.{}.{}.0", info.active.major,
                                     info.active.minor, info.active.point),
                         args.cleanOutput);
    }

    if (args.checkStageState)
    {
        auto findState = stateToString.find(info.state);
        std::string_view state = "CORRUPTED";
        if (findState != stateToString.end())
        {
            state = findState->second;
        }
        message += print("Status Staged State", state, args.cleanOutput);
    }

    if (args.otherInfo)
    {
        message += print("Staging Index", std::to_string(info.stagingIndex),
                         args.cleanOutput);

        message += print("CR51 Descriptor Hash",
                         hashToString(std::vector<uint8_t>(
                             &info.descriptorHash[0],
                             &info.descriptorHash[0] + SHA256_DIGEST_LENGTH)),
                         args.cleanOutput);
    }

    return message;
}
} // namespace info
} // namespace flashupdate
