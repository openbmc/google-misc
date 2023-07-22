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

#include <fmt/format.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sdbusplus/bus.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
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
constexpr auto MAC_INTERFACE = "xyz.openbmc_project.Network.MACAddress";
constexpr auto NETWORK_SERVICE = "xyz.openbmc_project.Network";
constexpr auto PROP_INTERFACE = "org.freedesktop.DBus.Properties";

int parse_mac(const std::string& mac_addr, mac_addr_t* mac)
{
    int ret = sscanf(mac_addr.c_str(), MAC_FORMAT, mac->octet, mac->octet + 1,
                     mac->octet + 2, mac->octet + 3, mac->octet + 4,
                     mac->octet + 5);

    return ret < 6 ? -1 : 0;
}

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

sdbusplus::message_t PhosphorConfig::new_networkd_call(sdbusplus::bus_t* dbus,
                                                       bool get) const
{
    auto networkd_call =
        dbus->new_method_call(NETWORK_SERVICE, iface_path_.c_str(),
                              PROP_INTERFACE, get ? "Get" : "Set");

    networkd_call.append(MAC_INTERFACE, "MACAddress");

    return networkd_call;
}

int PhosphorConfig::get_mac_addr(mac_addr_t* mac)
{
    if (mac == nullptr)
    {
        fmt::print(stderr, "mac is nullptr\n");
        return -1;
    }

    // Cache hit: we have stored host MAC.
    if (shared_host_mac_)
    {
        *mac = shared_host_mac_.value();
    }
    else // Cache miss: read MAC over DBus, and store in cache.
    {
        std::string mac_string;
        try
        {
            auto networkd_call = new_networkd_call(&bus, true);
            auto reply = bus.call(networkd_call);
            std::variant<std::string> result;
            reply.read(result);
            mac_string = std::get<std::string>(result);
        }
        catch (const sdbusplus::exception::SdBusError& ex)
        {
            fmt::print(stderr, "Failed to get MACAddress: {}\n", ex.what());
            return -1;
        }

        if (parse_mac(mac_string, mac) < 0)
        {
            fmt::print(stderr, "Failed to parse MAC Address `{}`\n",
                       mac_string);
            return -1;
        }

        shared_host_mac_ = *mac;
    }

    return 0;
}

int PhosphorConfig::set_mac_addr(const mac_addr_t& mac)
{
    auto networkd_call = new_networkd_call(&bus, false);
    std::variant<std::string> mac_value(format_mac(mac));
    networkd_call.append(mac_value);

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
        fmt::print(stderr, "Failed to set MAC Addr `{}` writing file: {}\n",
                   std::get<std::string>(mac_value), ex.what());
        return -1;
    }

    try
    {
        auto reply = bus.call(networkd_call);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        fmt::print(stderr, "Failed to set MAC Addr `{}`: {}\n",
                   std::get<std::string>(mac_value), ex.what());
        return -1;
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
        fmt::print(stderr, "Failed to set systemd nic status: {}\n", ex.what());
        return 1;
    }
}

} // namespace net
