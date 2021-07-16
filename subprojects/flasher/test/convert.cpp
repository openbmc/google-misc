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

#include <flasher/convert.hpp>

#include <gtest/gtest.h>

namespace flasher
{

TEST(Convert, Uint32)
{
    EXPECT_EQ(toUint32("0x40"), 64);
    EXPECT_EQ(toUint32("40"), 40);
    EXPECT_EQ(toUint32("0"), 0);

    EXPECT_THROW(toUint32(""), std::runtime_error);
    EXPECT_THROW(toUint32(","), std::runtime_error);
    EXPECT_THROW(toUint32(",0"), std::runtime_error);
    EXPECT_THROW(toUint32("0,"), std::runtime_error);
}

} // namespace flasher
