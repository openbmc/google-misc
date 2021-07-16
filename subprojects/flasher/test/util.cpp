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

#include <flasher/util.hpp>
#include <stdplus/fd/intf.hpp>
#include <stdplus/types.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace flasher
{

using testing::ElementsAre;
using testing::Return;

constexpr std::byte operator""_b(unsigned long long t)
{
    return static_cast<std::byte>(t);
}

struct Op
{
    MOCK_METHOD(stdplus::span<const std::byte>, op,
                (stdplus::span<const std::byte>, size_t offset), ());
};

TEST(OpAtExact, NoData)
{
    testing::StrictMock<Op> op;
    EXPECT_CALL(op, op(ElementsAre(1_b), 0))
        .WillOnce(Return(stdplus::span<std::byte>{}));
    EXPECT_THROW(opAtExact("op", &Op::op, op,
                           stdplus::span<const std::byte>(std::vector{1_b}), 0),
                 stdplus::exception::WouldBlock);
}

TEST(OpAtExact, FillSingle)
{
    testing::StrictMock<Op> op;
    auto data = std::vector{1_b};
    auto data_s = stdplus::span<std::byte>(data);
    EXPECT_CALL(op, op(ElementsAre(1_b), 3)).WillOnce(Return(data_s));
    opAtExact("op", &Op::op, op, data_s, 3);
}

TEST(OpAtExact, FillMulti)
{
    testing::StrictMock<Op> op;
    testing::InSequence seq;
    auto data = std::vector{1_b, 2_b};
    auto data_s = stdplus::span<std::byte>(data);
    EXPECT_CALL(op, op(ElementsAre(1_b, 2_b), 3))
        .WillOnce(Return(data_s.subspan(0, 1)));
    EXPECT_CALL(op, op(ElementsAre(2_b), 4))
        .WillOnce(Return(data_s.subspan(0, 1)));
    opAtExact("op", &Op::op, op, data_s, 3);
}

TEST(OpAtExact, NoFill)
{
    testing::StrictMock<Op> op;
    testing::InSequence seq;
    auto data = std::vector{1_b, 2_b};
    auto data_s = stdplus::span<std::byte>(data);
    EXPECT_CALL(op, op(ElementsAre(1_b, 2_b), 0))
        .WillOnce(Return(data_s.subspan(0, 1)));
    EXPECT_CALL(op, op(ElementsAre(2_b), 1))
        .WillOnce(Return(data_s.subspan(0, 0)));
    EXPECT_THROW(opAtExact("op", &Op::op, op, data_s, 0),
                 stdplus::exception::WouldBlock);
}

struct Fd
{
    MOCK_METHOD(off_t, lseek, (off_t, stdplus::fd::Whence), ());
    MOCK_METHOD(stdplus::span<std::byte>, op, (stdplus::span<std::byte>), ());
};

TEST(OpAt, NoSeek)
{
    testing::StrictMock<Fd> fd;
    auto data = std::vector{1_b, 2_b};
    auto data_s = stdplus::span<std::byte>(data);
    size_t cur_offset = 3;
    EXPECT_CALL(fd, op(ElementsAre(1_b, 2_b)))
        .WillOnce(Return(data_s.subspan(0, 1)));
    EXPECT_THAT(opAt(&Fd::op, fd, cur_offset, data_s, 3), ElementsAre(1_b));
    EXPECT_EQ(cur_offset, 4);
}

TEST(OpAt, Seek)
{
    testing::StrictMock<Fd> fd;
    testing::InSequence seq;
    auto data = std::vector{1_b, 2_b};
    auto data_s = stdplus::span<std::byte>(data);
    size_t cur_offset = 3;
    EXPECT_CALL(fd, lseek(5, stdplus::fd::Whence::Set)).WillOnce(Return(5));
    EXPECT_CALL(fd, op(ElementsAre(1_b, 2_b))).WillOnce(Return(data_s));
    EXPECT_THAT(opAt(&Fd::op, fd, cur_offset, data_s, 5),
                ElementsAre(1_b, 2_b));
    EXPECT_EQ(cur_offset, 7);
}

} // namespace flasher
