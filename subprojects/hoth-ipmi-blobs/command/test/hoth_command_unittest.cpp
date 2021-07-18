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

#include "hoth_command_unittest.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::UnorderedElementsAreArray;

namespace ipmi_hoth
{

class HothCommandBasicTest : public HothCommandTest
{};

TEST_F(HothCommandBasicTest, PathToHothId)
{
    EXPECT_EQ("", hvn.pathToHothId("/dev/hoth/command_passthru"));
    EXPECT_EQ("prologue",
              hvn.pathToHothId("/dev/hoth/prologue/command_passthru"));
}

TEST_F(HothCommandBasicTest, HothIdToPath)
{
    EXPECT_EQ("/dev/hoth/command_passthru", hvn.hothIdToPath(""));
    EXPECT_EQ("/dev/hoth/prologue/command_passthru",
              hvn.hothIdToPath("prologue"));
}

TEST_F(HothCommandBasicTest, CanHandleBlobChecksNameInvalid)
{
    // Verify canHandleBlob checks and returns false on an invalid name.

    EXPECT_FALSE(hvn.canHandleBlob("asdf"));
    EXPECT_FALSE(hvn.canHandleBlob("dev/hoth/command_passthru"));
    EXPECT_FALSE(hvn.canHandleBlob("/dev/hoth/command_passthru2"));
    EXPECT_FALSE(hvn.canHandleBlob("/dev/hoth/prologue/t/command_passthru"));
    EXPECT_FALSE(hvn.canHandleBlob("/dev/hoth/firmware_update"));
}

TEST_F(HothCommandBasicTest, CanHandleBlobChecksNameValid)
{
    // Verify canHandleBlob checks and returns true on the valid name.

    EXPECT_TRUE(hvn.canHandleBlob("/dev/hoth/command_passthru"));
    EXPECT_TRUE(hvn.canHandleBlob("/dev/hoth/prologue/command_passthru"));
}

TEST_F(HothCommandBasicTest, VerifyBehaviorOfBlobIds)
{
    // Verify the correct BlobIds is returned from the hoth command handler.
    internal::Dbus::SubTreeMapping mapping = {
        {"/xyz/openbmc_project/Control", {}},
        {"/xyz/openbmc_project/Control/Hoth2nologue", {}},
        {"/xyz/openbmc_project/Control/Hoth/nologue/2", {}},
        {"/xyz/openbmc_project/Control/Hoth", {}},
        {"/xyz/openbmc_project/Control/Hoth/prologue", {}},
        {"/xyz/openbmc_project/Control/Hoth/demidome", {}},
    };
    EXPECT_CALL(dbus, getHothdMapping()).WillOnce(Return(mapping));
    std::vector<std::string> expected = {
        "/dev/hoth/command_passthru",
        "/dev/hoth/prologue/command_passthru",
        "/dev/hoth/demidome/command_passthru",
    };
    EXPECT_THAT(hvn.getBlobIds(), UnorderedElementsAreArray(expected));
}

} // namespace ipmi_hoth
