/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "net_iface.h"
#include "net_sockio.h"

#include <linux/if_packet.h>

#include <cstddef>

namespace ncsi
{

class SockIO : public net::SockIO
{
  public:
    SockIO() = default;

    explicit SockIO(int sockfd) : net::SockIO(sockfd) {}

    // This function creates a raw socket and initializes sockfd_.
    // If the default constructor for this class was used,
    // this function MUST be called before the object can be used
    // for anything else.
    int init();

    // Since raw packet socket is used for NC-SI, it needs to be bound
    // to the interface. This function needs to be called after init,
    // before the socket it used for communication.
    int bind_to_iface(const net::IFaceBase& iface);

    // Applies a filter to the interface to ignore VLAN tagged packets
    int filter_vlans();

    // Non-blocking version of recv. Uses poll with timeout.
    int recv(void* buf, size_t maxlen) override;

  private:
    struct sockaddr_ll sock_addr_;
    const int kpoll_timeout_ = 10;
};

} // namespace ncsi
