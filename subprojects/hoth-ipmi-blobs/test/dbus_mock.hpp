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

#include "dbus.hpp"

#include <ipmid/api.h>
#include <systemd/sd-bus.h>

#include <string_view>

#include <gmock/gmock.h>

sd_bus* ipmid_get_sd_bus_connection()
{
    return nullptr;
}

namespace ipmi_hoth
{
namespace internal
{

class DbusMock : public virtual Dbus
{
  public:
    MOCK_METHOD0(getHothdMapping, SubTreeMapping());
    MOCK_METHOD1(pingHothd, bool(std::string_view));
};

} // namespace internal

} // namespace ipmi_hoth
