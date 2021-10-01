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
#include <vector>

namespace flashupdate
{

/** @struct Config
 *  @brief BIOS update configuration populated with Json input file.
 */
struct Config
{
    struct Key
    {
        std::string prod;
        std::string dev;
    };

    struct Partition
    {
        Partition(){};

        std::string name;
        std::string location;
        std::optional<uint16_t> muxSelect;
    };

    struct Flash
    {
        Key validationKey;
        Partition primary;
        std::vector<Partition> secondary;
        uint8_t stagingIndex;
        std::string deviceId;
        std::string driver;
    };

    struct Eeprom
    {
        std::string path;
        uint32_t offset;
    };

    Flash flash;
    Eeprom eeprom;
};

/** @brief Parse json file to create the BIOS update configuration
 *
 * @param[in] configFile    optional file name for custom config
 * @param[in] stagingIndex  flash partition index to stage for secondary storage
 *
 * @return BIOS update configuration
 */
Config createConfig(std::optional<std::string> configFile,
                    uint8_t stagingIndex);

} // namespace flashupdate
