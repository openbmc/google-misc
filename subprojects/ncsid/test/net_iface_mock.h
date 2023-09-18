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

#include <net_iface.h>

#include <string>
#include <vector>

namespace mock
{

class IFace : public net::IFaceBase
{
  public:
    IFace() : net::IFaceBase("mock0") {}
    explicit IFace(const std::string& name) : net::IFaceBase(name) {}
    int bind_sock(int sockfd) const override;

    mutable std::vector<int> bound_socks;
    int index;
    mutable short flags = 0;

  private:
    int ioctl_sock(int sockfd, int request, struct ifreq* ifr) const override;
    int ioctl(int request, struct ifreq* ifr) const override;
};

} // namespace mock
