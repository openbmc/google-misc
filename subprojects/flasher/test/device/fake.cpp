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

#include <sys/mman.h>

#include <flasher/device.hpp>
#include <flasher/device/fake.hpp>
#include <flasher/file/memory.hpp>

#include <memory>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace flasher
{
namespace device
{

using testing::ElementsAre;

constexpr std::byte operator""_b(unsigned long long t)
{
    return static_cast<std::byte>(t);
}

TEST(FakeSimpleDevice, EraseWrite)
{
    file::Memory m;

    Fake f(m, Fake::Type::Simple, 0);
    EXPECT_EQ(f.getSize(), 0);
    EXPECT_EQ(f.getEraseSize(), 0);

    m.data.resize(8);
    f = Fake(m, Fake::Type::Simple, 0);
    EXPECT_EQ(f.getSize(), 8);
    EXPECT_EQ(f.getEraseSize(), 0);
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b}, 0).size(), 2);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0xf0_b, 0x00_b, 0x00_b, 0x00_b,
                                    0x00_b, 0x00_b, 0x00_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b, 0xff_b, 0x00_b}, 1).size(),
              4);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x0f_b, 0xf0_b, 0xff_b, 0x00_b,
                                    0x00_b, 0x00_b, 0x00_b));
    f.eraseBlocks(1, 1);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x0f_b, 0xf0_b, 0xff_b, 0x00_b,
                                    0x00_b, 0x00_b, 0x00_b));
}

TEST(FakeNorDevice, EraseWrite)
{
    file::Memory m;

    Fake f(m, Fake::Type::Nor, 8);
    EXPECT_EQ(f.getSize(), 0);
    EXPECT_EQ(f.getEraseSize(), 8);

    m.data.resize(8);
    f = Fake(m, Fake::Type::Nor, 4);
    EXPECT_EQ(f.getSize(), 8);
    EXPECT_EQ(f.getEraseSize(), 4);
    f.eraseBlocks(0, 2);
    EXPECT_THAT(m.data, ElementsAre(0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,
                                    0xff_b, 0xff_b, 0xff_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b}, 0).size(), 2);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0xf0_b, 0xff_b, 0xff_b, 0xff_b,
                                    0xff_b, 0xff_b, 0xff_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b, 0xff_b, 0x00_b}, 1).size(),
              4);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x00_b, 0xf0_b, 0xff_b, 0x00_b,
                                    0xff_b, 0xff_b, 0xff_b));
    f.eraseBlocks(1, 1);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x00_b, 0xf0_b, 0xff_b, 0xff_b,
                                    0xff_b, 0xff_b, 0xff_b));
}

TEST(OpenFakeDevice, RequiredArguments)
{
    EXPECT_THROW(openDevice(ModArgs("fake,/dev/null")), std::invalid_argument);
    EXPECT_THROW(openDevice(ModArgs("fake,type=nor,erase=4096")),
                 std::invalid_argument);
    EXPECT_THROW(openDevice(ModArgs("fake,type=nor,/dev/null")),
                 std::invalid_argument);
    EXPECT_THROW(openDevice(ModArgs("fake,erase=4096,/dev/null")),
                 std::invalid_argument);
}

TEST(OpenFakeDevice, OpenFake)
{
    auto dev = openDevice(ModArgs("fake,type=nor,erase=4096,/dev/null"));
    EXPECT_EQ(dev->getSize(), 0);
}

} // namespace device
} // namespace flasher
