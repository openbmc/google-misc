#include "hoth_command_unittest.hpp"

#include <sdbusplus/exception.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::ContainerEq;
using ::testing::IsEmpty;
using ::testing::NotNull;
using ::testing::Return;

using namespace std::literals;

namespace ipmi_hoth
{

auto static const test_str = "Hello, world!"s;
std::vector<uint8_t> static const test_buf(test_str.begin(), test_str.end());
auto static const test2_str = "Good morning, world!"s;
std::vector<uint8_t> static const test2_buf(test2_str.begin(), test2_str.end());

sdbusplus::SdBusMock sdbusIntf;

ACTION(SdbusThrow)
{
    EXPECT_CALL(sdbusIntf, sd_bus_error_set_errno(NotNull(), _))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbusIntf, sd_bus_error_is_set(NotNull())).WillOnce(Return(1));
    EXPECT_CALL(sdbusIntf, sd_bus_error_free(NotNull())).Times(1);

    throw sdbusplus::exception::SdBusError(0, "", &sdbusIntf);
}

class HothCommandCommitTest : public HothCommandTest
{
  protected:
    void expectValidCommand(std::string_view name,
                            const std::vector<uint8_t>& input)
    {
        EXPECT_CALL(dbus, SendHostCommand(name, ContainerEq(input), _))
            .WillOnce([&](auto, const auto&, auto cb) {
                this->cb = std::move(cb);
                return stdplus::Cancel(std::nullopt);
            });
    }
    internal::DbusCommand::Cb cb;
};

TEST_F(HothCommandCommitTest, InvalidSessionCommitIsRejected)
{
    // Verify the hoth command handler checks for a valid session.

    EXPECT_FALSE(hvn.commit(session_, std::vector<uint8_t>()));
}

TEST_F(HothCommandCommitTest, UnexpectedDataParam)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));

    EXPECT_FALSE(hvn.commit(session_, std::vector<uint8_t>({1, 2, 3})));
}

TEST_F(HothCommandCommitTest, DbusCallFail)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    expectValidCommand("", test_buf);

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
    // session, offset, data
    EXPECT_TRUE(hvn.write(session_, 0, test_buf));
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    cb(std::nullopt);

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::commit_error | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

TEST_F(HothCommandCommitTest, EmptyLegacyHoth)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    expectValidCommand("", {});

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
    // session, offset, data
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    cb(std::vector<uint8_t>{});

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(0, meta.size);
    EXPECT_EQ(blobs::StateFlags::committed | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

TEST_F(HothCommandCommitTest, EmptyNamedHoth)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(name)))
        .WillOnce(Return(true));
    expectValidCommand(name, {});

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), namedPath));
    // session, offset, data
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    cb(std::vector<uint8_t>{});

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(0, meta.size);
    EXPECT_EQ(blobs::StateFlags::committed | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

// Tests the full commit process with example data
TEST_F(HothCommandCommitTest, HappyPath)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    expectValidCommand("", test_buf);

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
    // session, offset, data
    EXPECT_TRUE(hvn.write(session_, 0, test_buf));
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    cb(test2_buf);

    std::vector<uint8_t> result = hvn.read(session_, 0, test2_buf.size());
    EXPECT_THAT(result, ContainerEq(test2_buf));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));

    EXPECT_EQ(blobs::StateFlags::committed | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

// Tests that repeated commits only result in one D-Bus call
TEST_F(HothCommandCommitTest, IdempotentSuccess)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    expectValidCommand("", {});

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
    // session, offset, data
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    cb(std::vector<uint8_t>{});

    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committed | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

// Tests that repeated commits will retry DBus calls if there is an error
TEST_F(HothCommandCommitTest, ErrorRetry)
{
    EXPECT_CALL(dbus, pingHothd(std::string_view(""))).WillOnce(Return(true));
    expectValidCommand("", {});

    EXPECT_TRUE(hvn.open(session_, hvn.requiredFlags(), legacyPath));
    // session, offset, data
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    blobs::BlobMeta meta;
    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    cb(std::nullopt);

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::commit_error | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);

    expectValidCommand("", {});
    EXPECT_TRUE(hvn.commit(session_, std::vector<uint8_t>()));

    EXPECT_TRUE(hvn.stat(session_, &meta));
    EXPECT_EQ(blobs::StateFlags::committing | blobs::StateFlags::open_read |
                  blobs::StateFlags::open_write,
              meta.blobState);
}

} // namespace ipmi_hoth
