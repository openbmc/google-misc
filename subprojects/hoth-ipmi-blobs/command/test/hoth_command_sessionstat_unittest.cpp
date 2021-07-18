#include "hoth_command_unittest.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

using ::testing::Return;

class HothCommandSessionStatTest : public HothCommandTest
{
  protected:
    blobs::BlobMeta meta_;
    // Initialize expected_meta_ with empty members
    blobs::BlobMeta expected_meta_ = {};
};

TEST_F(HothCommandSessionStatTest, InvalidSessionStatIsRejected)
{
    // Verify the hoth command handler checks for a valid session.

    EXPECT_FALSE(hvn.stat(session_, &meta_));
}

TEST_F(HothCommandSessionStatTest, SessionStatAlwaysInitialReadAndWrite)
{
    // Verify the session stat returns the information for a session.

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_TRUE(hvn.stat(session_, &meta_));

    expected_meta_.blobState =
        blobs::StateFlags::open_read | blobs::StateFlags::open_write;
    EXPECT_EQ(meta_, expected_meta_);
}

TEST_F(HothCommandSessionStatTest, AfterWriteMetadataLengthMatches)
{
    // Verify that after writes, the length returned matches.

    std::vector<uint8_t> data = {0x01};
    EXPECT_EQ(1, data.size());

    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_TRUE(hvn.write(session_, (hvn.maxBufferSize() - 1), data));

    EXPECT_TRUE(hvn.stat(session_, &meta_));

    // We wrote one byte to the last index, making the length the buffer size.
    expected_meta_.size = hvn.maxBufferSize();
    expected_meta_.blobState =
        blobs::StateFlags::open_read | blobs::StateFlags::open_write;
    EXPECT_EQ(meta_, expected_meta_);
}

} // namespace ipmi_hoth
