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

#include "btStateMachine.hpp"

#include "utils.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>

constexpr const bool debug = true;

BTStateMachine::BTStateMachine(bool hostAlreadyOn,
                               std::shared_ptr<DbusHandler> dh) :
    _dh(dh)
{
    if (loadJson(kFinalJson))
    {
        _dh->update(btJson);
    }
    else if (loadJson(kResumeJson))
    {
        // Do nothing
    }
    else
    {
        // Basically this section will only be entered once when you first
        // enable this feature. In any other cases, either kFinalJson or
        // kResumeJson should be existed.
        initJson(!hostAlreadyOn);
    }
}

SMResult BTStateMachine::next(uint8_t nextTimePoint)
{
    // Ensure the `next()` function is atomic.
    const std::lock_guard<std::mutex> lock(m);
    // Get timestamp.
    auto currentTime = getSoCMonatomicTimestamp();
    if (currentTime == std::nullopt)
    {
        std::cerr << "[ERROR]: Can not read getSoCMonatomicTimestamp"
                  << std::endl;
        return {SMErrors::smErrUnknownErr, 0};
    }
    if (debug)
    {
        std::cerr << "[DEBUG]: current timepoint = "
                  << btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint]
                         .get<uint16_t>()
                  << ", next timepoint = "
                  << static_cast<uint16_t>(nextTimePoint)
                  << ", timestamp = " << *currentTime << std::endl;
    }

    // If there is any addition abnormal power cycle happens.
    // It will trigger the state machine move to `kBIOSStart`.
    // So handle it outside of the switch section will be more easier.
    const std::string kNextTPStr = std::to_string(nextTimePoint);
    if (nextTimePoint == BTTimePoint::kBIOSStart)
    {
        if (btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] ==
            BTTimePoint::kOSUserEnd)
        {
            std::remove(kFinalJson);
            initJson();
        }
        btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
            nextTimePoint;
        // If btJson doesn't contain `kNextTPStr`, it will create it.
        // If state machine move to `kBIOSStart` more than
        // `kMaxInternalRebootCount` times. It will stop pushing the timestamp
        // into json to avoid exhausting the memory. Instead it will change the
        // last element.
        if (btJson[BTCategory::kTimePoint].contains(kNextTPStr) &&
            btJson[BTCategory::kTimePoint][kNextTPStr].size() >=
                kMaxInternalRebootCount)
        {
            btJson[BTCategory::kTimePoint][kNextTPStr].back() = *currentTime;
        }
        else
        {
            btJson[BTCategory::kTimePoint][kNextTPStr].push_back(*currentTime);
        }
        saveJson(kResumeJson);
        return {SMErrors::smErrNone, *currentTime};
    }

    // Normal process
    switch (btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint]
                .get<uint8_t>())
    {
        case BTTimePoint::kBMCStart:
            if (nextTimePoint == BTTimePoint::kOSUserDownEndReboot ||
                nextTimePoint == BTTimePoint::kOSUserDownEndHalt)
            {
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        case BTTimePoint::kOSUserDownEndReboot:
        case BTTimePoint::kOSUserDownEndHalt:
            if (nextTimePoint == BTTimePoint::kOSKernelDownEnd)
            {
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        case BTTimePoint::kOSKernelDownEnd:
            // This state only accept `kBIOSStart` as the next state.
            // It will be handled in the beginning of the `next()` funtion.
            return {SMErrors::smErrWrongOrder, *currentTime};
        // case BTTimePoint::kBMCDownEnd:
        // This case will be cover by a systemd service that record the
        // kBMCDownEnd to the json file directly after final.target.
        case BTTimePoint::kBIOSStart:
            if (nextTimePoint == BTTimePoint::kBIOSEnd)
            {
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        case BTTimePoint::kBIOSEnd:
            if (nextTimePoint == BTTimePoint::kNerfUserEnd)
            {
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        case BTTimePoint::kNerfUserEnd:
            if (nextTimePoint == BTTimePoint::kOSUserEnd)
            {
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                calcDurations();
                _dh->update(btJson);
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        case BTTimePoint::kOSUserEnd:
            if (nextTimePoint == BTTimePoint::kOSUserDownEndReboot ||
                nextTimePoint == BTTimePoint::kOSUserDownEndHalt)
            {
                std::remove(kFinalJson);
                initJson();
                btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] =
                    nextTimePoint;
                btJson[BTCategory::kTimePoint][kNextTPStr] = *currentTime;
                break;
            }
            return {SMErrors::smErrWrongOrder, *currentTime};
        default:
            // Should be impossible.
            return {SMErrors::smErrUnknownErr, *currentTime};
    }
    // If any error happened during the swtich section it will immediately
    // return error.

    saveJson(kResumeJson);
    // If it's the end of power cycle process, rename the file from
    // `resume.json` to `final.json`
    if (btJson[BTCategory::kRuntime][BTRuntime::kCurrentTimePoint] ==
        BTTimePoint::kOSUserEnd)
    {
        if (std::rename(kResumeJson, kFinalJson) != 0)
        {
            std::cerr << "[Error]: Can't rename " << kResumeJson << " to "
                      << kFinalJson << std::endl;
        }
    }

    return {SMErrors::smErrNone, *currentTime};
}

bool BTStateMachine::setDuration(std::string stage,
                                 uint64_t durationMicrosecond, bool isExtra)
{
    if (isExtra)
    {
        if (btJson[BTCategory::kDuration].contains(BTDuration::kExtra) &&
            btJson[BTCategory::kDuration][BTDuration::kExtra].size() >=
                kMaxExtraCnt)
        {
            return false;
        }
        btJson[BTCategory::kDuration][BTDuration::kExtra][stage] =
            durationMicrosecond;
    }
    else
    {
        btJson[BTCategory::kDuration][stage] = durationMicrosecond;
    }
    return true;
}

void BTStateMachine::calcDurations()
{
    // Get time point from json
    std::optional<uint64_t> osKernelDownEnd = std::nullopt;
    std::optional<uint64_t> osUserDownEndHalt = std::nullopt;
    std::optional<uint64_t> osUserDownEndReboot = std::nullopt;
    std::optional<uint64_t> bmcDownEnd = std::nullopt;
    std::optional<uint64_t> bmcStart = std::nullopt;
    std::optional<uint64_t> biosStart = std::nullopt;
    std::optional<uint64_t> biosEnd = std::nullopt;
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kOSKernelDownEnd)))
    {
        osKernelDownEnd =
            btJson[BTCategory::kTimePoint][BTTimePoint::kOSKernelDownEnd]
                .get<uint64_t>();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kOSUserDownEndHalt)))
    {
        osUserDownEndHalt =
            btJson[BTCategory::kTimePoint][BTTimePoint::kOSUserDownEndHalt]
                .get<uint64_t>();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kOSUserDownEndReboot)))
    {
        osUserDownEndReboot =
            btJson[BTCategory::kTimePoint][BTTimePoint::kOSUserDownEndReboot]
                .get<uint64_t>();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kBMCDownEnd)))
    {
        bmcDownEnd = btJson[BTCategory::kTimePoint][BTTimePoint::kBMCDownEnd]
                         .get<uint64_t>();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kBMCStart)))
    {
        bmcStart = btJson[BTCategory::kTimePoint][BTTimePoint::kBMCStart]
                       .get<uint64_t>();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kBIOSStart)))
    {
        auto& el = btJson[BTCategory::kTimePoint][BTTimePoint::kBIOSStart];
        biosStart = el.back();
        btJson[BTCategory::kStatistic][BTStatistic::kInternalRebootCount] =
            el.size();
    }
    if (btJson[BTCategory::kTimePoint].contains(
            std::to_string(BTTimePoint::kBIOSEnd)))
    {
        biosEnd = btJson[BTCategory::kTimePoint][BTTimePoint::kBIOSEnd]
                      .get<uint64_t>();
    }

    // Calculate duration
    if (osKernelDownEnd != std::nullopt)
    {
        if (osUserDownEndHalt != std::nullopt)
        {
            btJson[BTCategory::kDuration][BTDuration::kOSKernelDown] =
                *osKernelDownEnd - *osUserDownEndHalt;
            btJson[BTCategory::kDuration][BTDuration::kTotal] =
                0; // export 0 to D_total if it's a halt.
        }
        else if (osUserDownEndReboot != std::nullopt)
        {
            btJson[BTCategory::kDuration][BTDuration::kOSKernelDown] =
                *osKernelDownEnd - *osUserDownEndReboot;
        }
    }
    if (bmcDownEnd != std::nullopt && osKernelDownEnd != std::nullopt)
    {
        btJson[BTCategory::kDuration][BTDuration::kBMCDown] =
            *bmcDownEnd - *osKernelDownEnd;
    }
    if (biosStart != std::nullopt && bmcStart != std::nullopt)
    {
        btJson[BTCategory::kDuration][BTDuration::kBMC] =
            *biosStart - *bmcStart;
    }
    if (biosEnd != std::nullopt && biosStart != std::nullopt)
    {
        btJson[BTCategory::kDuration][BTDuration::kBIOS] =
            *biosEnd - *biosStart;
    }
    if (btJson[BTCategory::kDuration].contains(BTDuration::kTotal))
    {
        uint64_t total = btJson[BTCategory::kDuration][BTDuration::kTotal];
        if (btJson[BTCategory::kDuration].contains(BTDuration::kOSUserDown))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kOSUserDown]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kOSKernelDown))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kOSKernelDown]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kBMC))
        {
            total -=
                btJson[BTCategory::kDuration][BTDuration::kBMC].get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kBIOS))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kBIOS]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kNerfKernel))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kNerfKernel]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kNerfUser))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kNerfUser]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kOSKernel))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kOSKernel]
                         .get<uint64_t>();
        }
        if (btJson[BTCategory::kDuration].contains(BTDuration::kOSUser))
        {
            total -= btJson[BTCategory::kDuration][BTDuration::kOSUser]
                         .get<uint64_t>();
        }
    }
}

void BTStateMachine::initJson(bool isAC)
{
    btJson = {{BTCategory::kDuration, {}},
              {BTCategory::kTimePoint, {}},
              {BTCategory::kStatistic, {{BTStatistic::kIsACPowerCycle, isAC}}},
              {BTCategory::kRuntime,
               {{BTRuntime::kCurrentTimePoint, BTTimePoint::kBMCStart}}}};
}

void BTStateMachine::saveJson(const std::string& file)
{
    std::ofstream fout(file);
    fout << std::setw(4) << btJson << std::endl;
    fout.close();
}

bool BTStateMachine::loadJson(const std::string& file)
{
    std::ifstream fin(file);
    if (!fin.is_open())
    {
        return false;
    }
    fin >> btJson;
    fin.close();
    return true;
}
