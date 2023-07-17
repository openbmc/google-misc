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

#include "common_defs.h"
#include "net_iface.h"

#include <linux/filter.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#include <cstring>
#include <iterator>

namespace ncsi
{

int SockIO::init()
{
    RETURN_IF_ERROR(sockfd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)),
                    "ncsi::SockIO::init() failed");
    return 0;
}

int SockIO::bind_to_iface(const net::IFaceBase& iface)
{
    iface.set_sock_flags(sockfd_, IFF_PROMISC);

    std::memset(&sock_addr_, 0, sizeof(sock_addr_));

    sock_addr_.sll_family = AF_PACKET;
    sock_addr_.sll_protocol = htons(ETH_P_ALL);

    RETURN_IF_ERROR(iface.bind_sock(sockfd_, &sock_addr_),
                    "ncsi::SockIO::bind_to_iface failed");

    return 0;
}

/**
 * Drops VLAN tagged packets from a socket
 *
 * ld vlant
 * jneq #0, drop
 * ld proto
 * jneq #0x88f8, drop
 * ret #-1
 * drop: ret #0
 */
struct sock_filter vlan_remove_code[] = {
    {0x20, 0, 0, 0xfffff02c}, {0x15, 0, 3, 0x00000000},
    {0x20, 0, 0, 0xfffff000}, {0x15, 0, 1, 0x000088f8},
    {0x6, 0, 0, 0xffffffff},  {0x6, 0, 0, 0x00000000}};

struct sock_fprog vlan_remove_bpf = {
    std::size(vlan_remove_code),
    vlan_remove_code,
};

int SockIO::filter_vlans()
{
    return setsockopt(sockfd_, SOL_SOCKET, SO_ATTACH_FILTER, &vlan_remove_bpf,
                      sizeof(vlan_remove_bpf));
}

int SockIO::recv(void* buf, size_t maxlen)
{
    struct pollfd sock_pollfd
    {
        sockfd_, POLLIN | POLLPRI, 0
    };

    int ret = poll(&sock_pollfd, 1, kpoll_timeout_);
    if (ret > 0)
    {
        return ::recv(sockfd_, buf, maxlen, 0);
    }

    return ret;
}

} // namespace ncsi
