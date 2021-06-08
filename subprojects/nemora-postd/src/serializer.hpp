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
#include "nemora_types.hpp"

#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

using std::int32_t;
using std::size_t;
using std::uint64_t;

class Serializer
{
  public:
    static std::string Serialize(const NemoraDatagram* dgram);

  private:
    static std::string SerializeEvent(const NemoraEvent* event);

    static constexpr uint64_t NEMORA_EVENT_PB_MAGIC = 0x890ebd38ec325800;
};
