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

#include <flashupdate/config.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <optional>

namespace flashupdate
{

Config createConfig(std::optional<std::string> configFile, uint8_t stagingIndex)
{
    Config config;
    std::string name = "/usr/share/flash-update/config.json";
    if (configFile)
    {
        name = *configFile;
    }

    std::ifstream configJson(name);
    if (!configJson.is_open())
    {
        throw std::runtime_error(fmt::format("{} is missing", name));
    }

    auto data = nlohmann::json::parse(configJson, nullptr, false);
    if (data.is_discarded())
    {
        throw std::runtime_error("failed to parse the config.json");
    }
    auto flash = data.at("flash");
    auto validationKey = flash.at("validation_key");
    validationKey.at("prod").get_to(config.flash.validationKey.prod);
    validationKey.at("dev").get_to(config.flash.validationKey.dev);

    auto primary = flash.at("primary");
    primary.at("name").get_to(config.flash.primary.name);
    primary.at("location").get_to(config.flash.primary.location);

    auto muxSelect = primary.at("mux_select");
    if (muxSelect.is_null())
    {
        throw std::runtime_error(
            "mux_select for primary partition is required");
    }
    uint16_t mux;
    muxSelect.get_to(mux);
    config.flash.primary.muxSelect = mux;

    auto secondaries = flash.at("secondary");
    for (const auto& secondary : secondaries)
    {
        Config::Partition partition;

        secondary.at("name").get_to(partition.name);
        secondary.at("location").get_to(partition.location);

        auto muxSelect = secondary.at("mux_select");

        if (!muxSelect.is_null())
        {
            muxSelect.get_to(mux);
            partition.muxSelect = mux;
        }

        config.flash.secondary.emplace_back(partition);
    }

    if (stagingIndex >= secondaries.size())
    {
        throw std::runtime_error(fmt::format(
            "stage index is greater than or equal to the number of staging "
            "partitions: {} >= {}",
            stagingIndex, secondaries.size()));
    }
    config.flash.stagingIndex = stagingIndex;

    flash.at("device_id").get_to(config.flash.deviceId);
    flash.at("driver").get_to(config.flash.driver);

    auto eeprom = data.at("eeprom");
    eeprom.at("path").get_to(config.eeprom.path);
    eeprom.at("offset").get_to(config.eeprom.offset);
    return config;
}

} // namespace flashupdate
