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

#include <flashupdate/info.hpp>
#include <flashupdate/ops.hpp>

#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace flashupdate
{

class OperationTest : public ::testing::Test
{
  protected:
    OperationTest()
    {
        resetInfo();
    }

    void resetInfo()
    {
        updateInfo.active = info::Version(activeVersion);
        updateInfo.stage = info::Version(stageVersion);
        updateInfo.stagingIndex = 3;
        updateInfo.state = 2;
    }

    info::UpdateInfo updateInfo;
    std::string_view activeVersion = "10.11.12.13";
    std::string_view stageVersion = "4.3.2.1";
};

TEST_F(OperationTest, InfoPass)
{
    auto ptr = reinterpret_cast<std::byte*>(&updateInfo);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::UpdateInfo));

    std::string filename = "test.txt";
    std::ofstream testfile;
    std::string expectedOutput = "";

    testfile.open(filename, std::ios::out);
    testfile << buffer.data();
    testfile.flush();
    testfile.close();

    Args args;
    args.config.eeprom.path = filename;

    std::stringstream printBuffer;
    std::streambuf* old = std::cout.rdbuf(printBuffer.rdbuf());
    ops::info(args);
    EXPECT_EQ(printBuffer.str(), expectedOutput);
    printBuffer.clear();

    args.checkActiveVersion = true;
    ops::info(args);
    expectedOutput += fmt::format("Active Version: {}\n", activeVersion.data());
    EXPECT_EQ(printBuffer.str(), expectedOutput);
    printBuffer.clear();

    args.checkStageVersion = true;
    ops::info(args);
    expectedOutput += fmt::format("Stage Version: {}\n", stageVersion.data());
    EXPECT_EQ(printBuffer.str(), expectedOutput);
    printBuffer.clear();

    args.checkStageState = true;
    ops::info(args);
    expectedOutput += "Status Staged State: CORRUPTED\n";
    EXPECT_EQ(printBuffer.str(), expectedOutput);
    printBuffer.clear();

    args.otherInfo = true;
    ops::info(args);
    expectedOutput += "Staging Index: 0\n";
    expectedOutput += "CR51 Descriptor Hash: 00000000000000000000000\n";
    EXPECT_EQ(printBuffer.str(), expectedOutput);

    args.cleanOutput = true;
    ops::info(args);
    expectedOutput =
        fmt::format("{}\n{}\n{}\n{}\n{}\n", activeVersion, stageVersion,
                    "CORRUPTED", "0", "00000000000000000000000");
    EXPECT_EQ(printBuffer.str(), expectedOutput);

    std::cout.rdbuf(old);
}

TEST(OperationTest, UpdateStateInvalidState)
{
    Args args;

    args.state = "FAKE_STATE";
    EXPECT_THROW(
        try { ops::updateState(args); } catch (const std::runtime_error& e) {
            EXPECT_STREQ(
                e.what(),
                fmt::format(
                    "{} is not a supported state. Need to be one of\n{}",
                    args.state)
                    .c_str());
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, UpdateStatePass)
{
    auto ptr = reinterpret_cast<std::byte*>(&updateInfo);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::UpdateInfo));

    std::string filename = "test.txt";
    std::ofstream testfile;
    std::string expectedOutput = "";

    testfile.open(filename, std::ios::out);
    testfile << buffer.data();
    testfile.flush();
    testfile.close();

    Args args;
    args.config.eeprom.path = filename;
    args.checkActiveVersion = true;
    args.checkStageVersion = true;
    args.checkStageState = true;
    args.otherInfo = true;
    args.cleanOutput = true;
    args.state = "STAGED";

    ops::updateState(args);

    std::stringstream printBuffer;
    std::streambuf* old = std::cout.rdbuf(printBuffer.rdbuf());
    ops::info(args);
    EXPECT_EQ(printBuffer.str(),
              fmt::format("{}\n{}\n{}\n{}\n{}\n", activeVersion, stageVersion,
                          "STAGED", "0", "00000000000000000000000"));
    printBuffer.clear();
    std::cout.rdbuf(old);
}

} // namespace flashupdate
