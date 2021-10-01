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

#include <flashupdate/config.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

#include <gtest/gtest.h>

namespace flashupdate
{

TEST(ConfigTest, EmptyConfig)
{
    std::ofstream testfile;
    testfile.open("empty.json", std::ios::out);
    auto empty = R"({})"_json;
    testfile << empty.dump();
    testfile.flush();

    EXPECT_THROW(createConfig("empty.json", 0), nlohmann::detail::out_of_range);
}

TEST(ConfigTest, ValidConfigWithMissingPirmaryMuxSelect)
{
    std::ofstream testfile;
    testfile.open("missing_primary_mux.json", std::ios::out);
    auto valid = R"(
        {
            "flash": {
                "validation_key": {
                    "prod": "prod.pem",
                    "dev": "dev.pem"
                },
                "primary": {
                    "name": "primary",
                    "location": "mtd,/dev/mtd1",
                    "mux_select": null
                },
                "secondary": [
                    {
                        "name": "secondary0",
                        "location": "mtd,/dev/mtd2",
                        "mux_select": null
                    }
                ],
                "device_id": "device_id",
                "driver": "/tmp/driver"
            },
            "eeprom": {
                "path": "eeprom",
                "offset": 0
            }
        }
    )"_json;
    testfile << valid.dump();
    testfile.flush();

    EXPECT_THROW(createConfig("missing_primary_mux.json", 0),
                 std::runtime_error);
}

TEST(ConfigTest, ValidConfig)
{
    std::ofstream testfile;
    testfile.open("valid.json", std::ios::out);
    auto valid = R"(
        {
            "flash": {
                "validation_key": {
                    "prod": "prod.pem",
                    "dev": "dev.pem"
                },
                "primary": {
                    "name": "primary",
                    "location": "mtd,/dev/mtd1",
                    "mux_select": 1
                },
                "secondary": [
                    {
                        "name": "secondary0",
                        "location": "mtd,/dev/mtd2",
                        "mux_select": null
                    },
                    {
                        "name": "secondary1",
                        "location": "mtd,/dev/mtd3",
                        "mux_select": 2
                    }
                ],
                "device_id": "device_id",
                "driver": "/tmp/driver"
            },
            "eeprom": {
                "path": "eeprom",
                "offset": 128
            }
        }
    )"_json;
    testfile << valid.dump();
    testfile.flush();

    // Test Invalid secondary index
    // secondary index > count(secondaries)
    EXPECT_THROW(createConfig("valid.json", 3), std::runtime_error);

    // Test Validate Json Config
    auto config = createConfig("valid.json", 1);
    // BIOS
    EXPECT_EQ(config.flash.deviceId, "device_id");
    EXPECT_EQ(config.flash.driver, "/tmp/driver");
    EXPECT_EQ(config.flash.stagingIndex, 1);

    // BIOS Public Key
    EXPECT_EQ(config.flash.validationKey.prod, "prod.pem");
    EXPECT_EQ(config.flash.validationKey.dev, "dev.pem");

    // BIOS Primary
    EXPECT_EQ(config.flash.primary.name, "primary");
    EXPECT_EQ(config.flash.primary.location, "mtd,/dev/mtd1");
    EXPECT_EQ(config.flash.primary.muxSelect, 1);

    // BIOS Secondary
    ASSERT_EQ(config.flash.secondary.size(), 2);
    EXPECT_EQ(config.flash.secondary[0].name, "secondary0");
    EXPECT_EQ(config.flash.secondary[0].location, "mtd,/dev/mtd2");
    EXPECT_EQ(config.flash.secondary[0].muxSelect, std::nullopt);
    EXPECT_EQ(config.flash.secondary[1].name, "secondary1");
    EXPECT_EQ(config.flash.secondary[1].location, "mtd,/dev/mtd3");
    EXPECT_EQ(config.flash.secondary[1].muxSelect, 2);

    // EEPROM
    EXPECT_EQ(config.eeprom.offset, 128);
    EXPECT_EQ(config.eeprom.path, "eeprom");
}

} // namespace flashupdate
