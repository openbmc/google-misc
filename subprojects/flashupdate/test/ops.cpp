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

TEST(OperationTest, InfoPass)
{
    info::UpdateInfo updateInfo;
    std::string_view activeVersion = "10.11.12.13";
    std::string_view stageVersion = "4.3.2.1";
    updateInfo.active = info::Version(activeVersion);
    updateInfo.stage = info::Version(stageVersion);
    updateInfo.stagingIndex = 3;
    updateInfo.state = 2;

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

    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkActiveVersion = true;
    expectedOutput += fmt::format("Active Version: {}\n", activeVersion.data());
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkStageVersion = true;
    expectedOutput += fmt::format("Stage Version: {}\n", stageVersion.data());
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkStageState = true;
    expectedOutput += "Status Staged State: CORRUPTED\n";
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.otherInfo = true;
    expectedOutput += "Staging Index: 0\n";
    expectedOutput += "CR51 Descriptor Hash: 00000000000000000000000\n";
    EXPECT_EQ(ops::info(args), expectedOutput);

    expectedOutput =
        fmt::format("{}\n{}\n{}\n{}\n{}\n", activeVersion, stageVersion,
                    "CORRUPTED", "0", "00000000000000000000000");
    EXPECT_EQ(ops::info(args), expectedOutput);
}

} // namespace flashupdate
