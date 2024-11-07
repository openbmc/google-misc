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

#include "net_config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/if_arp.h>
#include <unistd.h>

#include <sdbusplus/bus.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/print.hpp>
#include <stdplus/util/string.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <utility>
#include <variant>

/* Most of the code for interacting with DBus is from
 * phosphor-host-ipmid/utils.cpp
 */

namespace net
{

namespace
{

constexpr auto IFACE_ROOT = "/xyz/openbmc_project/network/";
constexpr auto MAC_FORMAT = "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx";
// 2 chars for every byte + 5 colons + Null byte
constexpr auto MAC_FORMAT_LENGTH = 6 * 2 + 5 + 1;
constexpr auto MAC_ADDR_LENGTH = 6;
constexpr auto MAC_INTERFACE = "xyz.openbmc_project.Network.MACAddress";
constexpr auto NETWORK_SERVICE = "xyz.openbmc_project.Network";
constexpr auto PROP_INTERFACE = "org.freedesktop.DBus.Properties";


std::string format_mac(const mac_addr_t& mac)
{
    // 2 chars for every byte + 5 colons + Null byte
    char mac_str[MAC_FORMAT_LENGTH];
    snprintf(mac_str, sizeof(mac_str), MAC_FORMAT, mac.octet[0], mac.octet[1],
             mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]);

    return std::string{mac_str};
}

} // namespace

PhosphorConfig::PhosphorConfig(const std::string& iface_name) :
    iface_name_{iface_name}, iface_path_{std::string(IFACE_ROOT) + iface_name},
    shared_host_mac_(std::experimental::nullopt),
    bus(sdbusplus::bus::new_default())
{}

int PhosphorConfig::get_mac_addr(mac_addr_t* mac)
{
    if (mac == nullptr)
    {
        stdplus::println(stderr, "mac is nullptr");
        return -1;
    }
    struct ifreq s;

    // Cache hit: we have stored host MAC.
    if (shared_host_mac_)
    {
        *mac = shared_host_mac_.value();
    }
    else // Cache miss: read from interface, cache it for future requests.
    {
        stdplus::ManagedFd fd{socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)};
        if (fd.get() == -1) {
          stdplus::println(stderr, "Failed to get socket fd to read MAC Address");
          return -1;
        }

        int ret = 0;
        strcpy(s.ifr_name, iface_name_.c_str());
        ret = ioctl(fd.get(), SIOCGIFHWADDR, &s);
        if (ret != 0) {
          stdplus::println(stderr, "Failed to ioctl for some reason {}", ret);
          return -1;
        }
        memcpy(mac, &s.ifr_addr.sa_data, sizeof(MAC_ADDR_LENGTH));
        shared_host_mac_ = *mac;
    }

    return 0;
}

int PhosphorConfig::set_mac_addr(const mac_addr_t& mac)
{
    std::variant<std::string> mac_value(format_mac(mac));
    struct ifreq ifr;
    int ret;
    strcpy(ifr.ifr_name, iface_name_.c_str());
    // Setting mac address on the interface struct
    ifr.ifr_hwaddr.sa_data[0] = mac.octet[0];
    ifr.ifr_hwaddr.sa_data[1] = mac.octet[1];
    ifr.ifr_hwaddr.sa_data[2] = mac.octet[2];
    ifr.ifr_hwaddr.sa_data[3] = mac.octet[3];
    ifr.ifr_hwaddr.sa_data[4] = mac.octet[4];
    ifr.ifr_hwaddr.sa_data[5] = mac.octet[5];

    try
    {
        auto netdir = std::format("/run/systemd/network/00-bmc-{}.network.d",
                                  iface_name_);
        std::filesystem::create_directories(netdir);
        auto netfile = std::format("{}/60-ncsi-mac.conf", netdir);
        auto fd = stdplus::fd::open(
            netfile,
            stdplus::fd::OpenFlags(stdplus::fd::OpenAccess::WriteOnly)
                .set(stdplus::fd::OpenFlag::Create),
            0644);
        auto contents = std::format("[Link]\nMACAddress={}\n",
                                    std::get<std::string>(mac_value));
        stdplus::fd::writeExact(fd, contents);
    }
    catch (const std::exception& ex)
    {
        stdplus::println(stderr, "Failed to set MAC Addr `{}` writing file: {}",
                         std::get<std::string>(mac_value), ex.what());
        return -1;
    }

    stdplus::ManagedFd s{socket(AF_INET, SOCK_DGRAM, 0)};
    if (s.get() == -1){
      stdplus::println(stderr, "Failed to open socket to set MAC Addr");
      return -1;
    }
    // Try setting MAC Address directly without bringing interface down 
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    ret = ioctl(s.get(), SIOCSIFHWADDR, &ifr);
    if (ret == -1) {
      // Bring interface down and trying to set MAC Address again.
      ifr.ifr_flags &= ~IFF_UP;
      ret = ioctl(s.get(), SIOCSIFFLAGS, &ifr);
      if (ret == -1){
       stdplus::println(stderr, "Failed bringing interface down to set mac address");
       return -1;
      }
      // Set MAC Address
      ret = ioctl(s.get(), SIOCSIFHWADDR, &ifr);
      if (ret == -1){
       stdplus::println(stderr, "Failed to set mac address after bringing interface down.");
      }
      // Bring interface up after setting MAC Address
      ifr.ifr_flags |= IFF_UP;
      ret = ioctl(s.get(), SIOCSIFFLAGS, &ifr);
      if (ret == -1){
       stdplus::println(stderr, "Failed bringing interface up after setting mac");
       return -1;
      }
    }
    shared_host_mac_ = std::experimental::nullopt;
    return 0;
}

int PhosphorConfig::set_nic_hostless(bool is_nic_hostless)
{
    // Ensure that we don't trigger the target multiple times. This is
    // undesirable because it will cause any inactive services to re-trigger
    // every time we run this code. Since the loop calling this executes this
    // code every 1s, we don't want to keep re-executing services. A fresh
    // start of the daemon will always trigger the service to ensure system
    // consistency.
    if (was_nic_hostless_ && is_nic_hostless == *was_nic_hostless_)
    {
        return 0;
    }

    static constexpr auto systemdService = "org.freedesktop.systemd1";
    static constexpr auto systemdRoot = "/org/freedesktop/systemd1";
    static constexpr auto systemdInterface = "org.freedesktop.systemd1.Manager";

    auto method = bus.new_method_call(systemdService, systemdRoot,
                                      systemdInterface, "StartUnit");
    if (is_nic_hostless)
    {
        method.append(
            stdplus::util::strCat("nic-hostless@", iface_name_, ".target"));
    }
    else
    {
        method.append(
            stdplus::util::strCat("nic-hostful@", iface_name_, ".target"));
    }

    // Specify --job-mode (see systemctl(1) for detail).
    method.append("replace");

    try
    {
        bus.call_noreply(method);
        was_nic_hostless_ = is_nic_hostless;
        return 0;
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        stdplus::println(stderr, "Failed to set systemd nic status: {}",
                         ex.what());
        return 1;
    }
}

} // namespace net
