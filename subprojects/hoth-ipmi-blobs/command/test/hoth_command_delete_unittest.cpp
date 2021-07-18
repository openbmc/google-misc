#include "hoth_command_unittest.hpp"

#include <gtest/gtest.h>

namespace ipmi_hoth
{

class HothCommandDeleteTest : public HothCommandTest
{};

TEST_F(HothCommandDeleteTest, VerifyHothDeleteFails)
{
    // The hoth command handler does not support delete.

    EXPECT_FALSE(hvn.deleteBlob(legacyPath));
}

} // namespace ipmi_hoth
