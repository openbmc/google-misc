#include <ec-ipmi-blobs/skm/handler.hpp>
#include <ipmid/handler.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace blobs
{
namespace ec
{
namespace skm
{

using testing::_;

class CmdMock : public Cmd
{
  public:
    MOCK_METHOD(stdplus::Cancel, exec,
                (uint8_t cmd, uint8_t ver,
                 stdplus::span<const std::byte> params, Cb cb),
                (noexcept, override));
};

class CancelMock : public stdplus::Cancelable
{
  public:
    MOCK_METHOD(void, cancel, (), (noexcept, override));
};

class HandlerTest : public testing::Test
{
  protected:
    HandlerTest() : h(cmd)
    {}

    void expectExec(Cmd::Cb& cb)
    {
        EXPECT_CALL(cmd, exec(_, _, _, _))
            .WillOnce([&](uint8_t, uint8_t, stdplus::span<const std::byte>,
                          Cmd::Cb icb) {
                cb = std::move(icb);
                return stdplus::Cancel(&cancel);
            });
    }

    testing::StrictMock<CancelMock> cancel;
    testing::StrictMock<CmdMock> cmd;
    Handler h;
};

TEST_F(HandlerTest, CanHandleBlob)
{
    EXPECT_FALSE(h.canHandleBlob(""));
    EXPECT_FALSE(h.canHandleBlob("/a"));
    EXPECT_FALSE(h.canHandleBlob("skm/hss/0"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss/"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss0"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss/a"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss/0a"));
    EXPECT_FALSE(h.canHandleBlob("/skm/hss/200"));

    EXPECT_TRUE(h.canHandleBlob("/skm/hss/0"));
    EXPECT_TRUE(h.canHandleBlob("/skm/hss/3"));
}

TEST_F(HandlerTest, GetBlobIds)
{
    EXPECT_THAT(h.getBlobIds(), testing::Contains("/skm/hss/"));
    EXPECT_THAT(h.getBlobIds(), testing::Contains("/skm/hss/1"));
}

TEST_F(HandlerTest, DeleteBlob)
{
    // Operation not supported by the underlying code
    EXPECT_THROW(h.deleteBlob("/skm/hss/1"), ipmi::HandlerCompletion);
}

TEST_F(HandlerTest, StatBlob)
{
    BlobMeta m;
    EXPECT_TRUE(h.stat("/skm/hss/1", &m));
    EXPECT_EQ(keySize, m.size);
    m = {};
    EXPECT_TRUE(h.stat("/skm/hss/0", &m));
    EXPECT_EQ(keySize, m.size);
}

TEST_F(HandlerTest, OpenNoRead)
{
    EXPECT_TRUE(h.open(0, OpenFlags::write, "/skm/hss/0"));
    BlobMeta m;
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, open_write);
}

TEST_F(HandlerTest, OpenDuplicate)
{
    EXPECT_TRUE(h.open(0, 0, "/skm/hss/0"));
    EXPECT_THROW(h.open(1, 0, "/skm/hss/0"), ipmi::HandlerCompletion);
    EXPECT_TRUE(h.close(0));
    EXPECT_TRUE(h.open(2, 0, "/skm/hss/0"));
}

TEST_F(HandlerTest, OpenReadWrite)
{
    Cmd::Cb cb;
    expectExec(cb);
    EXPECT_TRUE(h.open(0, OpenFlags::read | OpenFlags::write, "/skm/hss/0"));
    // Blob should now be open only as `comitting` to indicate it is still
    // reading
    BlobMeta m;
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing);

    // Cancel is always called for cleanup even after successful completion
    EXPECT_CALL(cancel, cancel());
    {
        std::array<std::byte, 2 + keySize> key = {};
        // Transition to fully open
        cb(0, key);
    }
    testing::Mock::VerifyAndClearExpectations(&cancel);
    // State should be updated for being open
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, open_read | open_write | committed);
}

TEST_F(HandlerTest, OpenReadError)
{
    Cmd::Cb cb;
    expectExec(cb);
    EXPECT_TRUE(h.open(0, OpenFlags::read | OpenFlags::write, "/skm/hss/0"));
    // Blob should now be open only as `comitting` to indicate it is still
    // reading
    BlobMeta m;
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing);

    // Cancel is always called for cleanup even after successful completion
    EXPECT_CALL(cancel, cancel());
    // Return an error from skm reading for open
    cb(1, {});
    testing::Mock::VerifyAndClearExpectations(&cancel);
    // State should only reflect the error opening
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, commit_error);
}

TEST_F(HandlerTest, OpenReadCancel)
{
    Cmd::Cb cb;
    expectExec(cb);
    EXPECT_TRUE(h.open(0, OpenFlags::read | OpenFlags::write, "/skm/hss/0"));
    // Blob should now be open only as `comitting` to indicate it is still
    // reading
    BlobMeta m;
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing);

    // Closing the blob should cancel the opening operation
    EXPECT_CALL(cancel, cancel());
    EXPECT_TRUE(h.close(0));
    testing::Mock::VerifyAndClearExpectations(&cancel);
}

TEST_F(HandlerTest, Read)
{
    // Read should fail for invalid session
    BlobMeta m;
    EXPECT_THROW(h.stat(0, &m), ipmi::HandlerCompletion);
    EXPECT_THROW(h.read(0, 0, 0), ipmi::HandlerCompletion);

    Cmd::Cb cb;
    expectExec(cb);
    EXPECT_TRUE(h.open(0, OpenFlags::read, "/skm/hss/0"));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing);
    // Reads should not be allowed while open is processing
    EXPECT_THROW(h.read(0, 0, 0), ipmi::HandlerCompletion);
    EXPECT_CALL(cancel, cancel());
    {
        std::array<std::byte, 2 + keySize> key;
        key[1] = key[0] = {};
        for (size_t i = 0; i < keySize; ++i)
        {
            key[i + 2] = std::byte{static_cast<uint8_t>(i)};
        }
        // Populate the key with an incrementing counter
        cb(0, key);
    }
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committed | open_read);
    // Out of bounds reads should fail
    EXPECT_THROW(h.read(0, 65, 64), ipmi::HandlerCompletion);
    // Truncated read at end boundary
    EXPECT_EQ((std::vector<uint8_t>{}), h.read(0, 64, 64));
    // Truncated read with some data
    EXPECT_EQ((std::vector<uint8_t>{60, 61, 62, 63}), h.read(0, 60, 64));
    // Other reads work
    EXPECT_EQ((std::vector<uint8_t>{4, 5, 6}), h.read(0, 4, 3));
}

TEST_F(HandlerTest, Write)
{
    // Write should fail for an invalid session
    BlobMeta m;
    EXPECT_THROW(h.stat(0, &m), ipmi::HandlerCompletion);
    EXPECT_THROW(h.write(0, 0, {}), ipmi::HandlerCompletion);

    Cmd::Cb cb;
    expectExec(cb);
    EXPECT_TRUE(h.open(0, OpenFlags::read | OpenFlags::write, "/skm/hss/0"));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing);
    // Writes should not be allowed while open is processing
    EXPECT_THROW(h.write(0, 0, {}), ipmi::HandlerCompletion);
    EXPECT_CALL(cancel, cancel());

    std::array<std::byte, 2 + keySize> key;
    key[1] = key[0] = {};
    for (size_t i = 0; i < keySize; ++i)
    {
        key[i + 2] = std::byte{static_cast<uint8_t>(i)};
    }
    // Populate the key with an incrementing counter
    cb(0, key);

    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committed | open_read | open_write);
    // Out of bounds writes fail
    EXPECT_THROW(h.write(0, 65, {}), ipmi::HandlerCompletion);
    // End of boundary write works
    EXPECT_TRUE(h.write(0, 64, {}));
    // Writing duplicate data considers the data `fresh`
    EXPECT_TRUE(h.write(0, 0, {0, 1, 2, 3, 4}));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committed | open_read | open_write);
    auto contents = h.read(0, 0, 64);
    ASSERT_EQ(64, contents.size());
    EXPECT_EQ(0, std::memcmp(contents.data(), &key[2], 64));

    // Basic write works
    EXPECT_TRUE(h.write(0, 0, {1, 1, 1}));
    EXPECT_EQ((std::vector<uint8_t>{1, 1, 1, 3}), h.read(0, 0, 4));
    // Write to the end works
    EXPECT_THROW(h.write(0, 63, {1, 2}), ipmi::HandlerCompletion);
    EXPECT_EQ((std::vector<uint8_t>{62, 1}), h.read(0, 62, 2));
    // Overwritten data is considered `stale`
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, open_read | open_write);
}

TEST_F(HandlerTest, WriteMeta)
{
    // Write meta is not a supported operation
    EXPECT_TRUE(h.open(0, OpenFlags::write, "/skm/hss/0"));
    EXPECT_THROW(h.writeMeta(0, 0, {}), ipmi::HandlerCompletion);
}

TEST_F(HandlerTest, Commit)
{
    // Commit should fail for an invalid session
    BlobMeta m;
    EXPECT_THROW(h.stat(0, &m), ipmi::HandlerCompletion);
    EXPECT_THROW(h.commit(0, {}), ipmi::HandlerCompletion);

    // Stage data for commit
    EXPECT_TRUE(h.open(0, OpenFlags::write, "/skm/hss/0"));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, open_write);
    EXPECT_TRUE(h.write(0, 1, {1, 1}));

    Cmd::Cb cb;
    expectExec(cb);
    // Start the commit
    EXPECT_TRUE(h.commit(0, {}));
    testing::Mock::VerifyAndClearExpectations(&cmd);
    // State should reflect a commit in progress
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing | open_write);
    // Commit a second time should coalesce with the ongoing commit
    EXPECT_TRUE(h.commit(0, {}));

    // Cancel is always called for cleanup
    EXPECT_CALL(cancel, cancel());
    // Our commit failed
    cb(1, {});
    testing::Mock::VerifyAndClearExpectations(&cancel);
    // State should reflect failure
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, commit_error | open_write);

    expectExec(cb);
    // Start the commit again after a failure
    EXPECT_TRUE(h.commit(0, {}));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committing | open_write);
    EXPECT_CALL(cancel, cancel());
    // Succeed this time
    cb(0, {});
    EXPECT_TRUE(h.stat(0, &m));
    // State should reflect success
    EXPECT_EQ(m.blobState, committed | open_write);

    // Committing again should be free
    EXPECT_TRUE(h.commit(0, {}));
    EXPECT_TRUE(h.stat(0, &m));
    EXPECT_EQ(m.blobState, committed | open_write);
}

TEST_F(HandlerTest, CloseNoSession)
{
    EXPECT_THROW(h.close(0), ipmi::HandlerCompletion);
}

TEST_F(HandlerTest, Expire)
{
    EXPECT_TRUE(h.open(0, 0, "/skm/hss/0"));
    EXPECT_TRUE(h.expire(1));
    EXPECT_TRUE(h.expire(0));
}

} // namespace skm
} // namespace ec
} // namespace blobs
