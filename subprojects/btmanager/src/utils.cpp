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

#include "utils.hpp"

#include <fstream>
#include <iostream>

std::optional<uint64_t> getSoCMonatomicTimestamp()
{
    // `/proc/uptime` starts counting when kernel is up, which means the elapsed
    // time before kernel won't be included.
    // TODO: Change to use
    // https://github.com/torvalds/linux/blob/master/arch/arm/include/asm/arch_timer.h#L108
    // to get more accurate T_0
    std::ifstream fin("/proc/uptime");
    if (!fin.is_open())
    {
        std::cerr << "[Error]: Can not read \"/proc/uptime\"" << std::endl;
        return std::nullopt;
    }
    double uptimeSec;
    fin >> uptimeSec;
    fin.close();

    return uptimeSec * 1000 > UINT64_MAX
               ? UINT64_MAX
               : static_cast<uint64_t>(uptimeSec * 1000);
}
