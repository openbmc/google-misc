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

#include <ipmid/handler.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;

namespace ipmi_hoth
{

class HothCommandReadTest : public HothCommandTest
{
  protected:
    const uint32_t testOffset_ = 0;
    const std::vector<uint8_t> testData_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    void openAndWriteTestData()
    {
        EXPECT_CALL(dbus, pingHothd(std::string_view("")))
            .WillOnce(Return(true));
        EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
        EXPECT_TRUE(hvn.write(session_, testOffset_, testData_));
    }
};

TEST_F(HothCommandReadTest, InvalidSessionReadIsRejected)
{
    // Verify that read checks for a valid session, returns empty buffer for
    // failed check.

    openAndWriteTestData();

    uint16_t wrongSession = session_ + 1;
    EXPECT_THROW(hvn.read(wrongSession, testOffset_, testData_.size()),
                 ipmi::HandlerCompletion);
}

TEST_F(HothCommandReadTest, ReadOffsetBeyondBufferSizeReturnsEmpty)
{
    // Verify that read with offset beyond buffer size returns empty buffer.

    openAndWriteTestData();

    uint32_t offsetBeyondBuffer = testData_.size();
    EXPECT_THAT(hvn.read(session_, offsetBeyondBuffer, testData_.size()),
                IsEmpty());
}

TEST_F(HothCommandReadTest, ReadFullWrittenData)
{
    // Verify that the read successfully reads back the written data.

    openAndWriteTestData();

    EXPECT_EQ(testData_, hvn.read(session_, testOffset_, testData_.size()));
}

TEST_F(HothCommandReadTest, ReadWrittenDataAtOffset)
{
    // Verify that the read with offset returns the expected data.

    openAndWriteTestData();

    EXPECT_EQ(testData_, hvn.read(session_, testOffset_, testData_.size()));

    // Try reading the written data byte by byte at each offset
    for (size_t i = 0; i < testData_.size(); ++i)
    {
        EXPECT_THAT(hvn.read(session_, i, 1), ElementsAre(testData_[i]));
    }
}

TEST_F(HothCommandReadTest, ReadFullWrittenDataWithBiggerRequestedSize)
{
    // Verify that read with requested size bigger than the written data will
    // return a response buffer up to the end of the written buffer.

    openAndWriteTestData();

    uint32_t requestedSizeBeyondBuffer = testData_.size() + 1;
    EXPECT_EQ(testData_,
              hvn.read(session_, testOffset_, requestedSizeBeyondBuffer));
}

TEST_F(HothCommandReadTest, ReadWrittenDataAtOffsetWithBiggerRequestedSize)
{
    // Verify that read with requested size bigger than the written data at an
    // offset will return a response buffer up to the end of the written buffer.

    openAndWriteTestData();

    uint32_t newOffset = testData_.size() / 2;
    uint32_t requestedSizeBeyondBuffer = testData_.size() + 1;
    std::vector<uint8_t> expectedData =
        std::vector<uint8_t>(testData_.begin() + newOffset, testData_.end());

    EXPECT_EQ(expectedData,
              hvn.read(session_, newOffset, requestedSizeBeyondBuffer));
}

} // namespace ipmi_hoth
