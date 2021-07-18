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

#include "dbus_command.hpp"
#include "dbus_mock.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

namespace ipmi_hoth
{
namespace internal
{

class DbusCommandMock : public DbusCommand, public DbusMock
{
  public:
    MOCK_METHOD(stdplus::Cancel, SendHostCommand,
                (std::string_view, const std::vector<uint8_t>&, Cb&&),
                (override));
};

} // namespace internal

} // namespace ipmi_hoth
