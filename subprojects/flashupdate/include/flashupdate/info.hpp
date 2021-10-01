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

#include <libcr51sign/libcr51sign_support.h>

#include <flashupdate/args.hpp>
#include <flashupdate/logging.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace flashupdate
{
namespace info
{

/** @struct Version
 *  @brief BIOS Version helper
 */
struct Version
{
    uint8_t major;
    uint8_t minor;
    uint8_t point;

    /** @brief Parse the version string and return valid parts
     *
     * @param[in, out] version   BIOS version string
     *
     * @return the leading number in the version string
     *
     * @throws std::runtime_error
     */
    uint8_t splitVersionChunk(std::string_view* version)
    {
        auto sep = version->find('.');
        if (sep == version->npos)
        {
            throw std::runtime_error(
                fmt::format("{}, Missing version separator", *version));
        }
        auto num = std::stoi(version->substr(0, sep).data());

        *version = version->substr(sep + 1);
        return num;
    }

    /** @brief Constructor for Version helper
     *
     * @param[in] version   BIOS version string
     */
    Version(std::string_view version)
    {
        major = splitVersionChunk(&version);
        minor = splitVersionChunk(&version);
        point = std::stoi(version.data());
    }
};

/** @struct UpdateInfo
 *  @brief BIOS update information
 */
struct UpdateInfo
{
    enum class State
    {
        CORRUPTED,
        STAGED,
        ACTIVATED,
        UPDATED,
        RECOVERY,
        RAM,
    };

    Version stage;
    Version active;
    uint8_t state;
    uint8_t descriptorHash[SHA256_DIGEST_LENGTH];
    uint8_t stagingIndex;
};

const std::unordered_map<std::string_view, UpdateInfo::State> stringToState = {
    {"CORRUPTED", UpdateInfo::State::CORRUPTED},
    {"STAGED", UpdateInfo::State::STAGED},
    {"ACTIVATED", UpdateInfo::State::ACTIVATED},
    {"UPDATED", UpdateInfo::State::UPDATED},
    {"RECOVERY", UpdateInfo::State::RECOVERY},
    {"RAM", UpdateInfo::State::RAM},
};

const std::unordered_map<uint8_t, std::string> stateToString = {
    {static_cast<uint8_t>(UpdateInfo::State::CORRUPTED), "CORRUPTED"},
    {static_cast<uint8_t>(UpdateInfo::State::STAGED), "STAGED"},
    {static_cast<uint8_t>(UpdateInfo::State::ACTIVATED), "ACTIVATED"},
    {static_cast<uint8_t>(UpdateInfo::State::UPDATED), "UPDATED"},
    {static_cast<uint8_t>(UpdateInfo::State::RECOVERY), "RECOVERY"},
    {static_cast<uint8_t>(UpdateInfo::State::RAM), "RAM"},
};

/** @brief Convert hash of cr51 descriptor to string
 *
 * @param[in] hash   hash value to convert to string
 *
 * @return string of the CR51 descriptor hash
 */
std::string hashToString(const std::vector<uint8_t>& hash);

/** @brief List all supported BIOS stage states
 *
 * @return list of BIOS states as string
 */
std::string listStates();

/** @brief Print BIOS UpdateInfo information
 *
 * @param[in] args    User input argument
 * @param[in] UpdateInfo  BIOS UpdateInfo to print
 */
void printUpdateInfo(const Args& args, struct UpdateInfo UpdateInfo);

} // namespace info
} // namespace flashupdate
