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

#include "net_iface.h"

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

namespace net
{

IFaceBase::IFaceBase(const std::string& name) : name_{name}
{
    if (name.size() >= IFNAMSIZ)
    {
        throw std::length_error("Interface name is too long");
    }
}

int IFaceBase::get_index() const
{
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    int ret = ioctl(SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        return ret;
    }

    return ifr.ifr_ifindex;
}

int IFaceBase::set_sock_flags(int sockfd, short flags) const
{
    return mod_sock_flags(sockfd, flags, true);
}

int IFaceBase::clear_sock_flags(int sockfd, short flags) const
{
    return mod_sock_flags(sockfd, flags, false);
}

int IFaceBase::mod_sock_flags(int sockfd, short flags, bool set) const
{
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));

    int ret = ioctl_sock(sockfd, SIOCGIFFLAGS, &ifr);
    if (ret < 0)
    {
        return ret;
    }

    if (set)
    {
        ifr.ifr_flags |= flags;
    }
    else
    {
        ifr.ifr_flags &= ~flags;
    }
    return ioctl_sock(sockfd, SIOCSIFFLAGS, &ifr);
}

int IFace::ioctl_sock(int sockfd, int request, struct ifreq* ifr) const
{
    if (ifr == nullptr)
    {
        return -1;
    }

    /* Avoid string truncation. */
    size_t len = name_.length();
    if (len + 1 >= sizeof(ifr->ifr_name))
    {
        return -1;
    }

    std::memcpy(ifr->ifr_name, name_.c_str(), len);
    ifr->ifr_name[len] = 0;

    return ::ioctl(sockfd, request, ifr);
}

int IFace::bind_sock(int sockfd) const
{
    struct sockaddr_ll saddr;
    std::memset(&saddr, 0, sizeof(saddr));
    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    saddr.sll_ifindex = get_index();
    return bind(sockfd, reinterpret_cast<struct sockaddr*>(&saddr),
                sizeof(saddr));
}

int IFace::ioctl(int request, struct ifreq* ifr) const
{
    if (ifr == nullptr)
    {
        return -1;
    }

    int tempsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tempsock < 0)
    {
        return tempsock;
    }

    int ret = ioctl_sock(tempsock, request, ifr);
    close(tempsock);

    return ret;
}

} // namespace net
