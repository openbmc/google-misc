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

#include "dbusHandler.hpp"

#include <btDefinitions.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

class DbusHandler;

enum class SMErrors
{
    smErrNone = 0x00,
    smErrWrongOrder,
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
    BTStateMachine(bool hostAlreadyOn, std::shared_ptr<DbusHandler> dh);

    SMResult next(uint8_t nextTimePoint);
    bool setDuration(std::string stage, uint64_t durationMicrosecond,
                     bool isExtra);

  private:
    const size_t kMaxExtraCnt = 100;
    const size_t kMaxBIOSStartTPCount = 1000;

    std::mutex m;
    nlohmann::json btJson;
    std::shared_ptr<DbusHandler> _dh;

    void calcDurations();
    void initJson(bool isAC = false);
    void saveJson(const std::string& file);
    bool loadJson(const std::string& file);
};
