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

#include <linux/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sdbusplus/bus.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/print.hpp>
#include <stdplus/raw.hpp>
#include <stdplus/util/string.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <thread>
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

    // Cache hit: we have stored host MAC.
    if (shared_host_mac_)
    {
        *mac = shared_host_mac_.value();
    }
    else // Cache miss: read from interface, cache it for future requests.
    {
        struct ifreq ifr = {};
        try
        {
            auto fd = stdplus::fd::socket(stdplus::fd::SocketDomain::INet6,
                                          stdplus::fd::SocketType::Datagram,
                                          stdplus::fd::SocketProto::IP);
            call_nic(fd, ifr, SIOCGIFHWADDR);
        }
        catch (const std::exception& ex)
        {
            stdplus::println(
                stderr,
                "Failed to get MAC Addr for Interface {} writing file: {}",
                iface_name_, ex.what());
            return -1;
        }
        std::copy_n(ifr.ifr_addr.sa_data, sizeof(*mac), mac->octet);
        shared_host_mac_ = *mac;
    }
    return 0;
}

void PhosphorConfig::call_nic(auto fd, struct ifreq& ifr, int op)
{
    std::copy_n(iface_name_.c_str(), iface_name_.size() + 1, ifr.ifr_name);
    fd.ioctl(op, &ifr);
}

int PhosphorConfig::set_mac_addr(const mac_addr_t& mac)
{
    std::string mac_value = format_mac(mac);
    struct ifreq ifr = {};
    short flags_copy;
    mac_addr_t cur_mac;
    int ret;
    ret = get_mac_addr(&cur_mac);
    if (ret == 0)
    {
        if (stdplus::raw::equal(cur_mac, mac))
        {
            // mac value is the same not doing anything, returning
            return 0;
        }
    }

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
        auto contents = std::format("[Link]\nMACAddress={}\n", mac_value);
        stdplus::fd::writeExact(fd, contents);
    }
    catch (const std::exception& ex)
    {
        stdplus::println(stderr, "Failed to set MAC Addr `{}` writing file: {}",
                         mac_value, ex.what());
        return -1;
    }
    try
    {
        auto fd = stdplus::fd::socket(stdplus::fd::SocketDomain::INet6,
                                      stdplus::fd::SocketType::Datagram,
                                      stdplus::fd::SocketProto::IP);
        // Try setting MAC Address directly without bringing interface down
        try
        {
            std::copy_n(mac.octet, 6, ifr.ifr_hwaddr.sa_data);
            call_nic(fd, ifr, SIOCSIFHWADDR);
        }
        catch (const std::exception& e)
        {
            // Regardless of error attempt to set MAC Address again after
            // bringing interface down
            stdplus::println(
                stderr,
                "Could not set MAC Address directly, retrying after bringing interface down, error = {}",
                e.what());
            try
            {
                // Read interface flags configuration and store (once interface
                // is brought down, existing state is lost)
                call_nic(fd, ifr, SIOCGIFFLAGS);
                flags_copy = ifr.ifr_flags;
                // set interface down
                ifr.ifr_flags &= ~IFF_UP;
                call_nic(fd, ifr, SIOCSIFFLAGS);
                // Wait for 1 milliseconds - sometimes interface is still
                // going down
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                // set MAC Address
                ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
                std::copy_n(mac.octet, 6, ifr.ifr_hwaddr.sa_data);
                call_nic(fd, ifr, SIOCSIFHWADDR);
                // set interface up with the flags state prior to bringing
                // it down
                ifr.ifr_flags = flags_copy | IFF_UP;
                call_nic(fd, ifr, SIOCSIFFLAGS);
            }
            catch (const std::exception& e)
            {
                stdplus::println(
                    stderr, "Failed to set MAC Address {} writing file: {}",
                    mac_value, e.what());
                return -1;
            }
        }
    }
    catch (const std::exception& e)
    {
        stdplus::println(stderr, "Error creating socket: {}", e.what());
        return -1;
    }
    stdplus::println(stderr, "Success setting Mac address for {}: {}",
                     iface_name_, mac_value);
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
