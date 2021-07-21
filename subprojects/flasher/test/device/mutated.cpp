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

#include <flasher/device/mock.hpp>
#include <flasher/device/mutated.hpp>
#include <flasher/mutate/asymmetric.hpp>

#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace flasher
{
namespace device
{

using testing::_;
using testing::ElementsAre;

constexpr std::byte operator""_b(unsigned long long t)
{
    return static_cast<std::byte>(t);
}

class MutatedTest : public testing::Test
{
  protected:
    MutatedTest() : dev(Device::Type::Nor, 4, 4), mdev(mut, dev)
    {}

    testing::StrictMock<Mock> dev;
    mutate::Asymmetric mut;
    Mutated mdev;
};

TEST_F(MutatedTest, Read)
{
    std::array<std::byte, 4> buf;
    EXPECT_CALL(dev, readAt(_, 1))
        .WillOnce([](stdplus::span<std::byte> buf, size_t) {
            std::memset(buf.data(), 4, buf.size());
            return buf;
        });
    EXPECT_THAT(mdev.readAt(buf, 1), ElementsAre(3_b, 2_b, 1_b, 0_b));
}

TEST_F(MutatedTest, Write)
{
    std::array<std::byte, 4> data = {0_b, 0_b, 0_b, 0_b};
    EXPECT_CALL(dev, writeAt(_, 3))
        .WillOnce([](stdplus::span<const std::byte> data, size_t) {
            EXPECT_THAT(data, ElementsAre(3_b, 4_b, 5_b, 6_b));
            return data;
        });
    EXPECT_THAT(mdev.writeAt(data, 3), ElementsAre(0_b, 0_b, 0_b, 0_b));
}

TEST_F(MutatedTest, Erase)
{
    EXPECT_CALL(dev, eraseBlocks(3, 2));
    mdev.eraseBlocks(3, 2);
}

} // namespace device
} // namespace flasher
