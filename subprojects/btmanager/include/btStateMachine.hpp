#pragma once

#include "btDefinitions.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

enum class SMErrors
{
    smErrNone = 0x00,
    smErrWrongOrder,
    smErrUnsupportedTimepoint,
    smErrUnknownErr,
};

struct SMResult
{
    SMErrors err;
    uint64_t value;
};

constexpr const char* kHostOffJson =
    "/usr/share/btmanager/host-off-time-points.json";

class BTStateMachine
{
  public:
    BTStateMachine(bool hostAlreadyOn);

    SMResult next(uint8_t nextTimePoint);
    void setDuration(std::string stage, uint64_t durationMicrosecond);

    BTTimePoint getTimePoints();
    BTDuration getDurations();
    int32_t getInternalRebootCount();
    bool isACPowerCycle();

  private:
    std::mutex m;
    uint8_t currentTimePoint;
    nlohmann::json hostOffJson;

    BTTimePoint timePoints;
    BTDuration durations;
    bool acPowerCycle;

    void calcDurations();
    void saveHostOffJson();
};
