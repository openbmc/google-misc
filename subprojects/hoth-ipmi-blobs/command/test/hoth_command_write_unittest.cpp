#include "hoth_command_unittest.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

using ::testing::Return;

class HothCommandWriteTest : public HothCommandTest
{};

TEST_F(HothCommandWriteTest, InvalidSessionWriteIsRejected)
{
    // Verify the hoth command handler checks for a valid session.

    std::vector<uint8_t> data = {0x1, 0x2};

    EXPECT_FALSE(hvn.write(session_, 0, data));
}

TEST_F(HothCommandWriteTest, WritingTooMuchByOneByteFails)
{
    // Test the edge case of writing 1 byte too much with an offset of 0.
    // writing 1025 at offset 0 is invalid.

    int bytes = hvn.maxBufferSize() + 1;
    std::vector<uint8_t> data(0x11);
    data.resize(bytes);
    ASSERT_EQ(bytes, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_FALSE(hvn.write(session_, 0, data));
}

TEST_F(HothCommandWriteTest, WritingTooMuchByOffsetOfOne)
{
    // Test the edge case of writing 1024 bytes (which is fine) but at the
    // offset 1, which makes it go over by 1 byte.
    // writing 1024 at offset 1 is invalid.

    int bytes = hvn.maxBufferSize();
    std::vector<uint8_t> data(0x11);
    data.resize(bytes);
    ASSERT_EQ(bytes, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    // offset of 1.
    EXPECT_FALSE(hvn.write(session_, 1, data));
}

TEST_F(HothCommandWriteTest, WritingOneByteBeyondEndFromOffsetFails)
{
    // Test the edge case of writing to the last byte offset but trying to
    // write two bytes.
    // writing 2 bytes at 1023 is invalid.

    std::vector<uint8_t> data = {0x01, 0x02};
    ASSERT_EQ(2, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_FALSE(hvn.write(session_, (hvn.maxBufferSize() - 1), data));
}

TEST_F(HothCommandWriteTest, WritingOneByteAtOffsetBeyondEndFails)
{
    // Test the edge case of writing one byte but exactly one byte beyond the
    // buffer.
    // writing 1 byte at 1024 is invalid.

    std::vector<uint8_t> data = {0x01};
    ASSERT_EQ(1, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_FALSE(hvn.write(session_, hvn.maxBufferSize(), data));
}

TEST_F(HothCommandWriteTest, WritingFullBufferAtOffsetZeroSucceeds)
{
    // Test the case where you write the full buffer length at once to the 0th
    // offset.

    int bytes = hvn.maxBufferSize();
    std::vector<uint8_t> data = {0x01};
    data.resize(bytes);
    ASSERT_EQ(bytes, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_TRUE(hvn.write(session_, 0, data));
}

TEST_F(HothCommandWriteTest, WritingOneByteToTheLastOffsetSucceeds)
{
    // Test the case where you write the last byte.

    std::vector<uint8_t> data = {0x01};
    ASSERT_EQ(1, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_TRUE(hvn.write(session_, (hvn.maxBufferSize() - 1), data));
}

} // namespace ipmi_hoth
