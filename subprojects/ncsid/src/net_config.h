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

#include "platforms/nemora/portable/net_types.h"

#include <linux/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net_iface.h>
#include <thread>
#include <sdbusplus/bus.hpp>

#include <experimental/optional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

// The API for configuring and querying network.

namespace net
{

using DBusObjectPath = std::string;
using DBusService = std::string;
using DBusInterface = std::string;
using ObjectTree =
    std::map<DBusObjectPath, std::map<DBusService, std::vector<DBusInterface>>>;

class ConfigBase
{
  public:
    virtual ~ConfigBase() = default;

    virtual int get_mac_addr(mac_addr_t* mac) = 0;

    virtual int set_mac_addr(const mac_addr_t& mac) = 0;

    // Called each time is_nic_hostless state is sampled.
    virtual int set_nic_hostless(bool is_nic_hostless) = 0;
};

// Calls Socket Ioctl to obtain information about NIC (previously
// phosphor-networkd)
class PhosphorConfig : public ConfigBase
{
  public:
    explicit PhosphorConfig(const std::string& iface_name);

    // Reads the MAC address from socket interface or internal
    // cache, and store in the mac pointer.
    // Returns -1 if failed, 0 if succeeded.
    int get_mac_addr(mac_addr_t* mac) override;

    // Sets the MAC address over socket, and update internal
    // cache.
    // Returns -1 if failed, 0 if succeeded.
    int set_mac_addr(const mac_addr_t& mac) override;
    virtual int set_nic_hostless(bool is_nic_hostless) override;

  private:
    sdbusplus::message_t new_networkd_call(sdbusplus::bus_t* dbus,
                                           bool get = false) const;

    const std::string iface_name_;
    const std::string iface_path_;

    // Stores the currently configured nic state, if previously set
    std::optional<bool> was_nic_hostless_;

    // Function helper allows get_mac_addr and set_mac_addr to do
    // ioctl calls to get and set different states of NIC.
    void call_nic(auto fd, struct ifreq& ifr, int op);

    // The MAC address obtained from NIC.
    // ncsid will commit this MAC address over DBus to phosphor-networkd
    // and expect it to be persisted. If actual host MAC address changes or
    // BMC MAC address is overwritten, a daemon reboot is needed to reset
    // the MAC.
    //   Initialized to nullopt which evaluates to false. Once a value is
    // set, bool() evaluates to true.
    std::experimental::optional<mac_addr_t> shared_host_mac_;

    // List of outstanding pids for config jobs
    std::list<pid_t> running_pids_;

    // Holds a reference to the bus for issuing commands to update network
    // config
    sdbusplus::bus_t bus;
};

} // namespace net
