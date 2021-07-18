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

#include <string_view>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

using ::testing::Return;

class HothCommandOpenTest : public HothCommandTest
{};

TEST_F(HothCommandOpenTest, OpenWithBadFlagsFails)
{
    // Hoth command handler open requires both read & write set.

    EXPECT_FALSE(hvn.open(session_, blobs::OpenFlags::read, legacyPath));
}

TEST_F(HothCommandOpenTest, OpenWithNoHothd)
{
    // Hoth command handler open without a backing hoth daemon present.

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(false));
    EXPECT_FALSE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
}

TEST_F(HothCommandOpenTest, OpenEverythingSucceeds)
{
    // Hoth command handler open with everything correct.

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
}

TEST_F(HothCommandOpenTest, OpenEleventhSessionFails)
{
    // Hoth command handler only allows ten open sessions, verifies the 11th
    // fails.

    EXPECT_CALL(dbus, pingHothd(std::string_view("")))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(dbus, pingHothd(std::string_view(name)))
        .WillRepeatedly(Return(true));
    size_t sessId = 0;

    for (int i = 0; i < hvn.maxSessions(); i++)
    {
        EXPECT_TRUE(hvn.open(sessId++, hvn.requiredFlags(), legacyPath));
    }

    for (int i = 0; i < hvn.maxSessions() - 3; i++)
    {
        EXPECT_TRUE(hvn.open(sessId++, hvn.requiredFlags(), namedPath));
    }

    EXPECT_FALSE(hvn.open(sessId++, hvn.requiredFlags(), legacyPath));

    for (int i = hvn.maxSessions() - 3; i < hvn.maxSessions(); i++)
    {
        EXPECT_TRUE(hvn.open(sessId++, hvn.requiredFlags(), namedPath));
    }

    EXPECT_FALSE(hvn.open(sessId++, hvn.requiredFlags(), namedPath));
}

TEST_F(HothCommandOpenTest, CannotOpenSameSessionTwice)
{
    // Verify the hoth command handler won't let you open the same session
    // twice.

    EXPECT_CALL(dbus, pingHothd(std::string_view("")))
        .WillRepeatedly(Return(true));

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_FALSE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
}

} // namespace ipmi_hoth
