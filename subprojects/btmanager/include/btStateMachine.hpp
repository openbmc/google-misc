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

    uint8_t getCurrentTimePoint();

  private:
    std::mutex m;
    uint8_t currentTimePoint;
    nlohmann::json btJson;

    BTTimePoint timePoints;
    BTDuration durations;
    bool acPowerCycle;

    void calcDurations();
    void saveJson(const std::string& file);
};