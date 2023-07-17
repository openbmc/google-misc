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

#include "net_iface_mock.h"

#include <memory>

#include "gtest/gtest.h"
#include <gtest/gtest.h>

TEST(TestIFace, TestGetIndex)
{
    mock::IFace iface_mock;

    constexpr int test_index = 5;
    iface_mock.index = test_index;

    EXPECT_EQ(test_index, iface_mock.get_index());
}

TEST(TestIFace, TestSetClearFlags)
{
    mock::IFace iface_mock;

    const short new_flags = 0xab;
    iface_mock.set_sock_flags(0, new_flags);
    EXPECT_EQ(new_flags, new_flags & iface_mock.flags);
    iface_mock.clear_sock_flags(0, 0xa0);
    EXPECT_EQ(0xb, new_flags & iface_mock.flags);
}
