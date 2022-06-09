// Copyright 2022 Google LLC
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

#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

constexpr const char* kHostOffJson = "/usr/share/btmanager/host-off-time-points.json";
constexpr const char* kHostBootTimeJson = "/usr/share/btmanager/host-boot-time.json";

constexpr const char* kIsACPowerCycleJsonKey = "IsACPowerCycle";
constexpr const char* kInternalRebootCountJsonKey = "InternalRebootCount";

class BTTimePoint
{
  public:
    constexpr static const uint8_t kOSUserDownEndReboot = 0x00;
    constexpr static const uint8_t kOSUserDownEndHalt =
        kOSUserDownEndReboot + 1;
    constexpr static const uint8_t kBIOSEnd = kOSUserDownEndHalt + 1;
    constexpr static const uint8_t kNerfUserEnd = kBIOSEnd + 1;
    constexpr static const uint8_t kOSUserEnd = kNerfUserEnd + 1;
    constexpr static const uint8_t kOSKernelDownEnd = kOSUserEnd + 1;
    constexpr static const uint8_t kBMCDownEnd = kOSKernelDownEnd + 1;
    constexpr static const uint8_t kBMCStart = kBMCDownEnd + 1;
    constexpr static const uint8_t kBIOSStart = kBMCStart + 1;
    static const std::set<uint8_t> dbusOwnedTimePoint;

    // member variable
    const uint64_t bmcStart = 0; // aka T_0
    uint64_t osUserDownEndReboot;
    uint64_t osUserDownEndHalt;
    uint64_t osKernelDownEnd;
    uint64_t bmcDownEnd;
    uint64_t biosEnd;
    uint64_t nerfUserEnd;
    uint64_t osUserEnd;
    std::vector<uint64_t> biosStart;

    BTTimePoint();

    void clear();
};

class BTDuration
{
  public:
    static const std::string kOSUserDown;
    static const std::string kOSKernelDown;
    static const std::string kBMCDown;
    static const std::string kBMC;
    static const std::string kBIOS;
    static const std::string kNerfKernel;
    static const std::string kNerfUser;
    static const std::string kOSKernel;
    static const std::string kOSUser;
    static const std::string kUnmeasured;
    // Please be careful here is the duration set that dbus DOESN'T owned
    // Because we allow host send extra duration to BMC, so any duration name
    // that is not occupied can be treat as extra duration.
    static const std::set<std::string> dbusNotOwnedDuration;
    static const std::set<std::string> dbusOwnedKeyDuration;

    // member variable
    uint64_t osUserDown;
    uint64_t osKernelDown;
    uint64_t bmcDown;
    uint64_t bmc;
    uint64_t bios;
    uint64_t nerfKernel;
    uint64_t nerfUser;
    uint64_t osKernel;
    uint64_t osUser;
    uint64_t total;
    uint64_t unmeasured;
    std::map<std::string, uint64_t> extra;

    BTDuration();

    void clear();
};
