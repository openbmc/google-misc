// Copyright 2021 Google LLC
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
#include <fmt/format.h>

namespace flashupdate
{

enum class LogLevel : uint8_t
{
    Emerg = 0,
    Alert = 1,
    Crit = 2,
    Error = 3,
    Warning = 4,
    Notice = 5,
    Info = 6,
    Debug = 7,
};

extern LogLevel logLevel;

#define log(level, ...)                                                        \
    switch (0)                                                                 \
    default:                                                                   \
        if ((level > ::flashupdate::logLevel))                                 \
        {}                                                                     \
        else                                                                   \
            ::fmt::print(stderr, __VA_ARGS__)

} // namespace flashupdate
