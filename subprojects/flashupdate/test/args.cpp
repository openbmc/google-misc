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

#include <flashupdate/args.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace flashupdate
{

using flasher::ModArgs;

class ArgsTest : public ::testing::Test
{
  protected:
    ArgsTest()
    {
        createConfig();
    }

    void createConfig()
    {
        std::ofstream testfile;
        testfile.open(testName, std::ios::out);
        auto good = R"(
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
                        "name": "secondary3",
                        "location": "mtd,/dev/mtd3",
                        "mux_select": 2
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
        testfile << good.dump();
        testfile.flush();
    }

    Args vecArgs(std::vector<std::string> args)
    {
        // Setting required configuration file.
        // Default to `/usr/share/flash-update/config.json`
        args.push_back("-j");
        args.push_back(testName);

        std::vector<char*> argv;
        for (auto& arg : args)
            argv.push_back(arg.data());

        argv.push_back(nullptr);
        return Args(args.size(), argv.data());
    }

    std::string testName = "test-config.json";
};

TEST_F(ArgsTest, OpRequired)
{
    EXPECT_THROW(vecArgs({"flashupdate", "-v"}), std::runtime_error);
}

TEST_F(ArgsTest, ConfigRequired)
{
    std::vector<std::string> args = {"flashupdate", "validate_config"};
    std::vector<char*> argv;
    for (auto& arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);

    EXPECT_THROW(Args(args.size(), argv.data()), std::runtime_error);
    EXPECT_EQ(vecArgs({"flashupdate", "validate_config"}).configFile, testName);
}

TEST_F(ArgsTest, InjectPersistentTest)
{
    EXPECT_THROW(vecArgs({"flashupdate", "inject_persistent"}),
                 std::runtime_error);
    auto args = vecArgs({"flashupdate", "inject_persistent", "file"});

    EXPECT_EQ(args.op, Args::Op::InjectPersistent);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, HashDescriptor)
{
    EXPECT_THROW(vecArgs({"flashupdate", "hash_descriptor"}),
                 std::runtime_error);
    auto args = vecArgs({"flashupdate", "hash_descriptor", "file"});

    EXPECT_EQ(args.op, Args::Op::HashDescriptor);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, ReadTest)
{
    EXPECT_THROW(vecArgs({"flashupdate", "read"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flashupdate", "read", "primary"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flashupdate", "read", "other", "file"}),
                 std::runtime_error);

    auto args = vecArgs({"flashupdate", "read", "primary", "file"});
    EXPECT_EQ(args.op, Args::Op::Read);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, true);
    EXPECT_EQ(args.stagingIndex, 0);

    args = vecArgs({"flashupdate", "read", "secondary", "file"});
    EXPECT_EQ(args.op, Args::Op::Read);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, false);
    EXPECT_EQ(args.stagingIndex, 0);
}

TEST_F(ArgsTest, WriteTest)
{
    EXPECT_THROW(vecArgs({"flashupdate", "write"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flashupdate", "write", "file"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flashupdate", "write", "file", "other"}),
                 std::runtime_error);

    auto args = vecArgs({"flashupdate", "write", "file", "primary"});
    EXPECT_EQ(args.op, Args::Op::Write);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, true);
    EXPECT_EQ(args.stagingIndex, 0);

    args = vecArgs({"flashupdate", "write", "file", "secondary"});
    EXPECT_EQ(args.op, Args::Op::Write);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, false);
    EXPECT_EQ(args.stagingIndex, 0);
}

TEST_F(ArgsTest, UpdateStateTest)
{
    EXPECT_THROW(vecArgs({"flashupdate", "update_state"}), std::runtime_error);

    auto args = vecArgs({"flashupdate", "update_state", "state"});
    EXPECT_EQ(args.op, Args::Op::UpdateState);
    EXPECT_EQ(args.file, std::nullopt);
    EXPECT_EQ(args.state, "state");
}

TEST_F(ArgsTest, UpdateStagedVersionTest)
{
    EXPECT_THROW(vecArgs({"flashupdate", "update_staged_version"}),
                 std::runtime_error);

    auto args = vecArgs({"flashupdate", "update_staged_version", "file"});
    EXPECT_EQ(args.op, Args::Op::UpdateStagedVersion);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, Verbose)
{
    EXPECT_EQ(0, vecArgs({"flashupdate", "validate_config"}).verbose);
    EXPECT_EQ(
        4, vecArgs({"flashupdate", "--verbose", "-v", "validate_config", "-vv"}).verbose);
}

} // namespace flashupdate
