#include "hoth_command_unittest.hpp"

#include <string_view>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

using ::testing::Return;

class HothCommandCloseTest : public HothCommandTest
{};

TEST_F(HothCommandCloseTest, CloseWithInvalidSessionFails)
{
    // Verify you cannot close an invalid session.

    EXPECT_FALSE(hvn.close(session_));
}

TEST_F(HothCommandCloseTest, CloseWithValidSessionSuccess)
{
    // Verify you can close a valid session.

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_TRUE(hvn.close(session_));
}

} // namespace ipmi_hoth
