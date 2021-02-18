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

#include <linux/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <functional>
#include <string>

namespace net
{

class IFaceBase
{
  public:
    explicit IFaceBase(const std::string& name);
    virtual ~IFaceBase() = default;

    /** @brief Get the index of the network interface corresponding
     * to this object.
     */
    int get_index() const;

    /** @brief Set interface flags using provided socket.
     *
     *  @param[in] sockfd - Socket to use for SIOCSIFFLAGS ioctl call.
     *  @param[in] flags - Flags to set.
     */
    int set_sock_flags(int sockfd, short flags) const;

    /** @brief Clear interface flags using provided socket.
     *
     *  @param[in] sockfd - Socket to use for SIOCSIFFLAGS/SIOCGIFFLAGS
     *      ioctl call.
     *  @param[in] flags - Flags to clear.
     */
    int clear_sock_flags(int sockfd, short flags) const;

    /** @brief Bind given socket to this interface. Similar to bind
     *     syscall, except that it fills in sll_ifindex field
     *     of struct sockaddr_ll with the index of this interface.
     */
    virtual int bind_sock(int sockfd, struct sockaddr_ll* saddr) const = 0;

  protected:
    std::string name_;

  private:
    /** @brief Similar to ioctl syscall, but the socket is created inside
     *      the function and the interface name in struct ifreq is
     *      properly populated with the index of this interface.
     */
    virtual int ioctl(int request, struct ifreq* ifr) const = 0;

    /** @brief Similar to ioctl syscall. The interface index in
     *      struct ifreq is
     *      properly populated with the index of this interface.
     */
    virtual int ioctl_sock(int sockfd, int request,
                           struct ifreq* ifr) const = 0;

    /** @brief Modify interface flags, using the given socket for
     *      ioctl call.
     */
    int mod_sock_flags(int sockfd, short flags, bool set) const;
};

class IFace : public IFaceBase
{
  public:
    explicit IFace(const std::string& name) : IFaceBase(name)
    {}

    /** @brief Bind given socket to this interface. Similar to bind
     *     syscall, except that it fills in sll_ifindex field
     *     of struct sockaddr_ll with the index of this interface.
     */
    int bind_sock(int sockfd, struct sockaddr_ll* saddr) const override;

  private:
    /** @brief Similar to ioctl syscall, but the socket is created inside
     *      the function and the interface name in struct ifreq is
     *      properly populated with the index of this interface.
     */
    int ioctl(int request, struct ifreq* ifr) const override;
    /** @brief Similar to ioctl syscall. The interface index in
     *      struct ifreq is
     *      properly populated with the index of this interface.
     */
    int ioctl_sock(int sockfd, int request, struct ifreq* ifr) const override;
};

} // namespace net
