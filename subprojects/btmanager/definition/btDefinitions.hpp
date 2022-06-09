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
#include <set>
#include <string>

constexpr const char* kResumeJson = "/usr/share/btmanager/resume.json";
constexpr const char* kFinalJson = "/usr/share/btmanager/final.json";

class BTCategory
{
  public:
    static const std::string kDuration;
    static const std::string kTimePoint;
    static const std::string kStatistic;
    static const std::string kRuntime;
};

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
    static const std::string kExtra;
    static const std::string kTotal;
    // Please be careful here is the duration set that dbus DOESN'T owned
    // Because we allow host send extra duration to BMC, so any duration name
    // that is not occupied can be treat as extra duration.
    static const std::set<std::string> dbusNotOwnedDuration;
    static const std::set<std::string> dbusOwnedKeyDuration;
};

class BTStatistic
{
  public:
    static const std::string kIsACPowerCycle;
    static const std::string kInternalRebootCount;
};

class BTRuntime
{
  public:
    static const std::string kCurrentTimePoint;
};
