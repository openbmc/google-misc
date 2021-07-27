/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cli.hpp"

#include <stdlib.h>
#include <unistd.h>

#include <filesystem>

#include <gtest/gtest.h>

namespace
{

constexpr char kBinary[] = "/path/to/bios_validator";

using namespace google::bios_validator;
using std::filesystem::temp_directory_path;

class CliTest : public ::testing::Test
{
  protected:
    CommandLine cli;
    std::string kBIOSFile = temp_directory_path() / "fake_bios_XXXXXX";
    std::string kKeyFile = temp_directory_path() / "fake_key_XXXXXX";

    void SetUp() override
    {
        ASSERT_GE(mkstemp(kBIOSFile.data()), 0);
        ASSERT_GE(mkstemp(kKeyFile.data()), 0);
    }

    // Unlink temp files no matter they exist or not.
    // It is OK not to check the return values.
    void TearDown() override
    {
        unlink(kBIOSFile.c_str());
        unlink(kKeyFile.c_str());
    }
};

TEST_F(CliTest, MissSubcommand)
{
    std::array args{kBinary};
    EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
}

TEST_F(CliTest, WrongSubcommand)
{
    std::array args{kBinary, "nonexistent-cmd"};
    EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
}

TEST_F(CliTest, ValidatorOnSuccess)
{
    std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(), "100",
                    kKeyFile.c_str()};
    EXPECT_EQ(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    EXPECT_EQ(cli.gotSubcommand(kValidatorCMD), true);
    ValidatorArgs validatorArgs = cli.getArgs();
    EXPECT_STRCASEEQ(validatorArgs.biosFilename.c_str(), kBIOSFile.c_str());
    EXPECT_EQ(validatorArgs.biosFileSize, 100);
    EXPECT_STRCASEEQ(validatorArgs.keyFilename.c_str(), kKeyFile.c_str());
}

TEST_F(CliTest, ValidatorFileNotExist)
{
    // BIOS doesn't exist
    ASSERT_EQ(unlink(kBIOSFile.c_str()), 0);
    std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(), "100",
                    kKeyFile.c_str()};
    EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);

    // Key doesn't exist
    ASSERT_EQ(unlink(kKeyFile.c_str()), 0);
    EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
}

TEST_F(CliTest, ValidatorMissPositionalArgs)
{
    // miss one or more positional arguments
    {
        std::array args{kBinary, kValidatorCMD};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
    {
        std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str()};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
    {
        std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(), "100"};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
}

TEST_F(CliTest, ValidatorInvalidBIOSSize)
{
    // the input is not an integer
    {
        std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(),
                        "hello123world", kKeyFile.c_str()};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
    // too small
    {
        std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(), "-1",
                        kKeyFile.c_str()};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
    // too large
    {
        uint64_t size = std::numeric_limits<uint64_t>::max();
        std::string size_str = std::to_string(size);
        std::array args{kBinary, kValidatorCMD, kBIOSFile.c_str(),
                        size_str.c_str(), kKeyFile.c_str()};
        EXPECT_NE(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    }
}

TEST_F(CliTest, ValidatorWriteVersion)
{
    std::string versionFilename = "/tmp/imageVersion";
    std::array args{
        kBinary,          kValidatorCMD,     kBIOSFile.c_str(),      "100",
        kKeyFile.c_str(), "--write_version", versionFilename.c_str()};
    EXPECT_EQ(cli.parseArgs(args.size(), args.data()), EXIT_SUCCESS);
    EXPECT_EQ(cli.gotSubcommand(kValidatorCMD), true);
    ValidatorArgs validatorArgs = cli.getArgs();
    EXPECT_STRCASEEQ(validatorArgs.biosFilename.c_str(), kBIOSFile.c_str());
    EXPECT_EQ(validatorArgs.biosFileSize, 100);
    EXPECT_STRCASEEQ(validatorArgs.keyFilename.c_str(), kKeyFile.c_str());
    EXPECT_EQ(validatorArgs.writeVersion, true);
    EXPECT_STRCASEEQ(validatorArgs.versionFilename.c_str(),
                     versionFilename.c_str());
}

} // namespace
