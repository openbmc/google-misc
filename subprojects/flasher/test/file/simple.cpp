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

#include <flasher/file/simple.hpp>
#include <stdplus/fd/dupable.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/util/cexec.hpp>

#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace flasher
{
namespace file
{

using testing::ElementsAre;

constexpr std::byte operator""_b(unsigned long long t)
{
    return static_cast<std::byte>(t);
}

TEST(Simple, SimpleMethods)
{
    stdplus::DupableFd fd(
        CHECK_ERRNO(memfd_create("simple_test", 0), "memfd_create"));
    stdplus::fd::writeExact(fd, std::array<uint8_t, 4>{1, 2, 3, 4});
    Simple simple(fd);

    EXPECT_EQ(simple.getSize(), 4);
    std::array<std::byte, 5> data;
    EXPECT_THAT(simple.readAt(data, 0), ElementsAre(1_b, 2_b, 3_b, 4_b));
    EXPECT_THAT(simple.readAt(data, 2), ElementsAre(3_b, 4_b));

    simple.truncate(5);
    EXPECT_EQ(simple.getSize(), 5);
    EXPECT_THAT(simple.readAt(data, 0), ElementsAre(1_b, 2_b, 3_b, 4_b, 0_b));
    simple.truncate(3);
    EXPECT_EQ(simple.getSize(), 3);
    EXPECT_THAT(simple.readAt(data, 1), ElementsAre(2_b, 3_b));

    simple.writeAt(std::array<std::byte, 1>{10_b}, 1);
    EXPECT_EQ(simple.getSize(), 3);
    EXPECT_THAT(simple.readAt(data, 0), ElementsAre(1_b, 10_b, 3_b));

    simple.writeAt(std::array<std::byte, 1>{11_b}, 4);
    EXPECT_EQ(simple.getSize(), 5);
    EXPECT_THAT(simple.readAt(data, 0), ElementsAre(1_b, 10_b, 3_b, 0_b, 11_b));
}

} // namespace file
} // namespace flasher
