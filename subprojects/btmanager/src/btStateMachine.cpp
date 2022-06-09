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
        std::remove(kHostOffJson);
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
                hostOffJson[std::to_string(BTTimePoint::kOSUserDownEndReboot)] =
                    *currentTime;
                saveHostOffJson();
                break;
            }
            else if (nextTimePoint == BTTimePoint::kOSUserDownEndHalt)
            {
                currentTimePoint = nextTimePoint;
                timePoints.osUserDownEndHalt = *currentTime;
                hostOffJson[std::to_string(BTTimePoint::kOSUserDownEndHalt)] =
                    *currentTime;
                saveHostOffJson();
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
                hostOffJson[std::to_string(BTTimePoint::kOSKernelDownEnd)] =
                    *currentTime;
                saveHostOffJson();
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
                hostOffJson.clear();
                timePoints.clear();
                durations.clear();

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
}

void BTStateMachine::saveHostOffJson()
{
    std::ofstream fout(kHostOffJson);
    fout << std::setw(4) << hostOffJson << std::endl;
    fout.close();
}
