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

namespace mock
{

int IFace::bind_sock(int sockfd, struct sockaddr_ll*) const
{
    bound_socks.push_back(sockfd);
    return 0;
}

int IFace::ioctl_sock(int, int request, struct ifreq* ifr) const
{
    return ioctl(request, ifr);
}

int IFace::ioctl(int request, struct ifreq* ifr) const
{
    int ret = 0;
    switch (request)
    {
        case SIOCGIFINDEX:
            ifr->ifr_ifindex = index;
            break;
        case SIOCGIFFLAGS:
            ifr->ifr_flags = flags;
            break;
        case SIOCSIFFLAGS:
            flags = ifr->ifr_flags;
            break;
        default:
            ret = -1;
    }

    return ret;
}

} // namespace mock
