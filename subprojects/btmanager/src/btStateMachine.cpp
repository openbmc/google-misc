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

BTStateMachine::BTStateMachine(bool hostAlreadyOn)
{
    currentTimePoint = BTTimePoint::kBMCStart;
    acPowerCycle = hostAlreadyOn ? false : true;
    std::ifstream fin(kHostOffJson);
    if (fin.is_open())
    {
        nlohmann::json j;
        fin >> j;

        if (j.find(std::to_string(BTTimePoint::kOSUserDownEndHalt)) != j.end())
        {
            timePoints.osUserDownEndHalt =
                j[std::to_string(BTTimePoint::kOSUserDownEndHalt)];
        }
        if (j.find(std::to_string(BTTimePoint::kOSUserDownEndReboot)) !=
            j.end())
        {
            timePoints.osUserDownEndReboot =
                j[std::to_string(BTTimePoint::kOSUserDownEndReboot)];
        }
        if (j.find(std::to_string(BTTimePoint::kBMCDownEnd)) != j.end())
        {
            timePoints.bmcDownEnd = j[std::to_string(BTTimePoint::kBMCDownEnd)];
        }

        fin.close();
    }
}

SMResult BTStateMachine::next(uint8_t nextTimePoint)
{
    const std::lock_guard<std::mutex> lock(m);
    auto currentTime = getSoCMonatomicTimestamp();
    if (currentTime == std::nullopt)
    {
        std::cerr << "ERROR: Can not read getSoCMonatomicTimestamp"
                  << std::endl;
        return {SMErrors::smErrUnknownErr, 0};
    }
    SMResult result = {
        .err = SMErrors::smErrNone,
        .value = *currentTime,
    };
    switch (currentTimePoint)
    {
        case BTTimePoint::kBMCStart:
            if (nextTimePoint == BTTimePoint::kOSUserDownEndReboot)
            {
                currentTimePoint = nextTimePoint;
                timePoints.osUserDownEndReboot = *currentTime;
                btJson[std::to_string(BTTimePoint::kOSUserDownEndReboot)] =
                    *currentTime;
                saveJson(kHostOffJson);
                break;
            }
            else if (nextTimePoint == BTTimePoint::kOSUserDownEndHalt)
            {
                currentTimePoint = nextTimePoint;
                timePoints.osUserDownEndHalt = *currentTime;
                btJson[std::to_string(BTTimePoint::kOSUserDownEndHalt)] =
                    *currentTime;
                saveJson(kHostOffJson);
                break;
            }
            else if (nextTimePoint == BTTimePoint::kBIOSStart)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosStart.push_back(*currentTime);
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        case BTTimePoint::kOSUserDownEndReboot:
        case BTTimePoint::kOSUserDownEndHalt:
            if (nextTimePoint == BTTimePoint::kOSKernelDownEnd)
            {
                currentTimePoint = nextTimePoint;
                timePoints.osKernelDownEnd = *currentTime;
                btJson[std::to_string(BTTimePoint::kOSKernelDownEnd)] =
                    *currentTime;
                saveJson(kHostOffJson);
                next(BTTimePoint::kBMCStart); // Move the state to kBMCStart
                break;
            }
            else if (nextTimePoint == BTTimePoint::kBIOSStart)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosStart.push_back(*currentTime);
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        case BTTimePoint::kOSKernelDownEnd:
            if (nextTimePoint == BTTimePoint::kBMCStart)
            {
                currentTimePoint = nextTimePoint;
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        // case BTTimePoint::kBMCDownEnd:
        // This case will be cover by a systemd service that record the
        // kBMCDownEnd to the json file directly after final.target.
        case BTTimePoint::kBIOSStart:
            if (nextTimePoint == BTTimePoint::kBIOSEnd)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosEnd = *currentTime;
                break;
            }
            else if (nextTimePoint == BTTimePoint::kBIOSStart)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosStart.push_back(*currentTime);
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        case BTTimePoint::kBIOSEnd:
            if (nextTimePoint == BTTimePoint::kNerfUserEnd)
            {
                currentTimePoint = nextTimePoint;
                timePoints.nerfUserEnd = *currentTime;
                break;
            }
            else if (nextTimePoint == BTTimePoint::kBIOSStart)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosStart.push_back(*currentTime);
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        case BTTimePoint::kNerfUserEnd:
            if (nextTimePoint == BTTimePoint::kOSUserEnd)
            {
                currentTimePoint = nextTimePoint;
                timePoints.osUserEnd = *currentTime;
                calcDurations();
                saveJson(kHostBootTimeJson);
                break;
            }
            else if (nextTimePoint == BTTimePoint::kBIOSStart)
            {
                currentTimePoint = nextTimePoint;
                timePoints.biosStart.push_back(*currentTime);
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        case BTTimePoint::kOSUserEnd:
            if (nextTimePoint == BTTimePoint::kOSUserDownEndReboot ||
                nextTimePoint == BTTimePoint::kOSUserDownEndHalt ||
                nextTimePoint == BTTimePoint::kBIOSStart)
            {
                btJson.clear();
                timePoints.clear();
                durations.clear();
                std::remove(kHostBootTimeJson);
                std::remove(kHostOffJson);

                currentTimePoint = BTTimePoint::kBMCStart;
                result = next(nextTimePoint); // Move the state to kBMCStart
                break;
            }
            result.err = SMErrors::smErrWrongOrder;
            break;
        default:
            // Should be impossible.
            result.err = SMErrors::smErrUnsupportedTimepoint;
            break;
    }
    return result;
}

void BTStateMachine::setDuration(std::string stage,
                                 uint64_t durationMicrosecond)
{
    if (stage.compare(BTDuration::kOSUserDown) == 0)
    {
        durations.osUserDown = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kOSKernelDown) == 0)
    {
        durations.osKernelDown = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kBMCDown) == 0)
    {
        durations.bmcDown = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kBMC) == 0)
    {
        durations.bmc = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kBIOS) == 0)
    {
        durations.bios = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kNerfKernel) == 0)
    {
        durations.nerfKernel = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kNerfUser) == 0)
    {
        durations.nerfUser = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kOSKernel) == 0)
    {
        durations.osKernel = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kOSUser) == 0)
    {
        durations.osUser = durationMicrosecond;
    }
    else if (stage.compare(BTDuration::kUnmeasured) == 0)
    {
        durations.unmeasured = durationMicrosecond;
    }
    else
    {
        durations.extra[stage] = durationMicrosecond;
    }
}

BTTimePoint BTStateMachine::getTimePoints()
{
    return timePoints;
}

BTDuration BTStateMachine::getDurations()
{
    return durations;
}

int32_t BTStateMachine::getInternalRebootCount()
{
    return timePoints.biosStart.size();
}

bool BTStateMachine::isACPowerCycle()
{
    return acPowerCycle;
}

uint8_t BTStateMachine::getCurrentTimePoint()
{
    return currentTimePoint;
}

void BTStateMachine::calcDurations()
{
    durations.osKernelDown =
        timePoints.osKernelDownEnd - (timePoints.osUserDownEndHalt > 0
                                          ? timePoints.osUserDownEndHalt
                                          : timePoints.osUserDownEndReboot);
    durations.bmcDown = timePoints.bmcDownEnd - timePoints.osKernelDownEnd;
    if (timePoints.biosStart.size() != 0)
    {
        durations.bmc = timePoints.biosStart.back() - timePoints.bmcStart;
        durations.bios = timePoints.biosEnd - timePoints.biosStart.back();
    }
    else
    {
        std::cerr << "[ERROR]: BIOS time point doesn't exist!" << std::endl;
    }
    durations.unmeasured =
        durations.total - durations.osUserDown - durations.osKernelDown -
        durations.bmc - durations.bios - durations.nerfKernel -
        durations.nerfUser - durations.osKernel - durations.osUser;

    // Update json
    btJson[BTDuration::kOSKernelDown] = durations.osKernelDown;
    btJson[BTDuration::kBMCDown] = durations.bmcDown;
    btJson[BTDuration::kBMC] = durations.bmc;
    btJson[BTDuration::kBIOS] = durations.bios;
    btJson[BTDuration::kUnmeasured] = durations.unmeasured;
    btJson[kIsACPowerCycleJsonKey] = acPowerCycle;
    btJson[kInternalRebootCountJsonKey] = timePoints.biosStart.size(); 
}

void BTStateMachine::saveJson(const std::string& file)
{
    std::ofstream fout(file);
    fout << std::setw(4) << btJson << std::endl;
    fout.close();
}