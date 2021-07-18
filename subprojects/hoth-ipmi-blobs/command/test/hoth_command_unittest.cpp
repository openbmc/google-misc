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
