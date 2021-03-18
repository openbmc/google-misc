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

#include "ncsi_sockio.h"
#include "net_iface_mock.h"

#include <gmock/gmock.h>

TEST(TestSockIO, TestBind)
{
    mock::IFace iface_mock;
    constexpr int test_index = 5;
    iface_mock.index = test_index;

    // This needs to be negative so that ncsi::SockIO
    // won't try to close the socket upon desctrution.
    constexpr int sock_fake_fd = -10;
    ncsi::SockIO ncsi_sock(sock_fake_fd);

    ncsi_sock.bind_to_iface(iface_mock);
    EXPECT_THAT(iface_mock.bound_socks.size(), testing::Ge(0));
    EXPECT_THAT(iface_mock.bound_socks, testing::Contains(sock_fake_fd));
    EXPECT_EQ(iface_mock.flags & IFF_PROMISC, IFF_PROMISC);
}
