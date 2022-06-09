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

#include "btDefinitions.hpp"

BTTimePoint::BTTimePoint()
{
    clear();
}

void BTTimePoint::clear()
{
    osUserDownEndReboot = 0;
    osUserDownEndHalt = 0;
    osKernelDownEnd = 0;
    bmcDownEnd = 0;
    // bmcStart = 0; bmcStart is T_0, which means it must be 0.
    biosEnd = 0;
    nerfUserEnd = 0;
    osUserEnd = 0;
    biosStart.clear();
}

const std::set<uint8_t> BTTimePoint::dbusOwnedTimePoint = {
    BTTimePoint::kOSUserDownEndReboot,
    BTTimePoint::kOSUserDownEndHalt,
    BTTimePoint::kBIOSEnd,
    BTTimePoint::kNerfUserEnd,
    BTTimePoint::kOSUserEnd,
};

BTDuration::BTDuration()
{
    clear();
}

void BTDuration::clear()
{
    osUserDown = 0;
    osKernelDown = 0;
    bmcDown = 0;
    bmc = 0;
    bios = 0;
    nerfKernel = 0;
    nerfUser = 0;
    osKernel = 0;
    osUser = 0;
    total = 0;
    unmeasured = 0;
    extra.clear();
}

const std::string BTDuration::kOSUserDown = "OSUserDown";
const std::string BTDuration::kOSKernelDown = "OSKernelDown";
const std::string BTDuration::kBMCDown = "BMCDown";
const std::string BTDuration::kBMC = "BMC";
const std::string BTDuration::kBIOS = "BIOS";
const std::string BTDuration::kNerfKernel = "NerfKernel";
const std::string BTDuration::kNerfUser = "NerfUser";
const std::string BTDuration::kOSKernel = "OSKernel";
const std::string BTDuration::kOSUser = "OSUser";
const std::string BTDuration::kUnmeasured = "Unmeasured";
const std::set<std::string> BTDuration::dbusNotOwnedDuration = {
    BTDuration::kOSKernelDown, BTDuration::kBMCDown,    BTDuration::kBMC,
    BTDuration::kBIOS,         BTDuration::kUnmeasured,
};
const std::set<std::string> BTDuration::dbusOwnedKeyDuration = {
    BTDuration::kOSUserDown, BTDuration::kNerfKernel, BTDuration::kNerfUser,
    BTDuration::kOSKernel,   BTDuration::kOSUser,
};
