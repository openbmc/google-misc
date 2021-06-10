#include <ec-ipmi-blobs/skm/cmd.hpp>

#include <array>
#include <cstring>

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
    MOCK_METHOD(
        Cancel, exec,
        (uint8_t cmd, uint8_t ver, stdplus::span<const std::byte> params,
         fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)> cb),
        (noexcept, override));
};

TEST(CmdTest, ReadKeyCmdError)
{
    testing::StrictMock<CmdMock> cmd;
    EXPECT_CALL(cmd, exec(0xca, 0, _, _))
        .WillOnce(
            [&](uint8_t, uint8_t, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) {
                EXPECT_EQ(params.size(), 66);
                EXPECT_EQ(params[0], std::byte{2});
                EXPECT_EQ(params[1], std::byte{1});
                cb(1, {});
                return Cancel(std::nullopt);
            });
    readKey(cmd, 1,
            [&](uint8_t res, stdplus::span<std::byte>) { EXPECT_EQ(res, 1); });
}

TEST(CmdTest, ReadKeyCmdInvalidParams)
{
    testing::StrictMock<CmdMock> cmd;
    EXPECT_CALL(cmd, exec(0xca, 0, _, _))
        .WillOnce(
            [&](uint8_t, uint8_t, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) {
                EXPECT_EQ(params.size(), 66);
                EXPECT_EQ(params[0], std::byte{2});
                EXPECT_EQ(params[1], std::byte{0});
                cb(0, {});
                return Cancel(std::nullopt);
            });
    readKey(cmd, 0, [&](uint8_t res, stdplus::span<std::byte>) {
        EXPECT_EQ(res, resInvalidRsp);
    });
}

TEST(CmdTest, ReadKeySuccess)
{
    testing::StrictMock<CmdMock> cmd;
    std::array<std::byte, 2 + keySize> rsp;
    for (size_t i = 0; i < rsp.size(); ++i)
    {
        rsp[i] = static_cast<std::byte>(i);
    }
    EXPECT_CALL(cmd, exec(0xca, 0, _, _))
        .WillOnce(
            [&](uint8_t, uint8_t, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) {
                EXPECT_EQ(params.size(), 66);
                EXPECT_EQ(params[0], std::byte{2});
                EXPECT_EQ(params[1], std::byte{5});
                cb(0, rsp);
                return Cancel(std::nullopt);
            });
    readKey(cmd, 5, [&](uint8_t res, stdplus::span<std::byte> key) {
        EXPECT_EQ(res, 0);
        auto expected_key = stdplus::span<std::byte>(rsp).subspan(2);
        ASSERT_EQ(key.size(), keySize);
        EXPECT_EQ(std::memcmp(key.data(), expected_key.data(), keySize), 0);
    });
}

TEST(CmdTest, WriteKeyError)
{
    testing::StrictMock<CmdMock> cmd;
    EXPECT_CALL(cmd, exec(0xca, 0, _, _))
        .WillOnce(
            [&](uint8_t, uint8_t, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) {
                EXPECT_EQ(params.size(), 66);
                EXPECT_EQ(params[0], std::byte{1});
                EXPECT_EQ(params[1], std::byte{1});
                cb(1, {});
                return Cancel(std::nullopt);
            });
    writeKey(cmd, 1, {}, [&](uint8_t res) { EXPECT_EQ(res, 1); });
}

TEST(CmdTest, WriteKeySuccess)
{
    testing::StrictMock<CmdMock> cmd;
    std::array<std::byte, keySize> key;
    for (size_t i = 0; i < key.size(); ++i)
    {
        key[i] = static_cast<std::byte>(i);
    }
    EXPECT_CALL(cmd, exec(0xca, 0, _, _))
        .WillOnce(
            [&](uint8_t, uint8_t, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) {
                EXPECT_EQ(params.size(), 66);
                EXPECT_EQ(params[0], std::byte{1});
                EXPECT_EQ(params[1], std::byte{0});
                EXPECT_EQ(params.size(), keySize + 2);
                EXPECT_EQ(std::memcmp(&params[2], key.data(), keySize), 0);
                cb(0, {});
                return Cancel(std::nullopt);
            });
    writeKey(cmd, 0, key, [&](uint8_t res) { EXPECT_EQ(res, 0); });
}

} // namespace skm
} // namespace ec
} // namespace blobs
