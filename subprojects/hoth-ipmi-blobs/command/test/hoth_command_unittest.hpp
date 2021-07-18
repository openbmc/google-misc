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

#include "dbus_command_mock.hpp"
#include "hoth_command.hpp"

#include <stdplus/util/string.hpp>

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

class HothCommandTest : public ::testing::Test
{
  protected:
    explicit HothCommandTest() : hvn(&dbus)
    {}

    // dbus mock object
    testing::StrictMock<internal::DbusCommandMock> dbus;

    HothCommandBlobHandler hvn;

    const uint16_t session_ = 0;
    const std::string legacyPath = stdplus::util::strCat(
        HothCommandBlobHandler::pathPrefix, hvn.pathSuffix());
    const std::string name = "prologue";
    const std::string namedPath = stdplus::util::strCat(
        HothCommandBlobHandler::pathPrefix, name, "/", hvn.pathSuffix());
};

} // namespace ipmi_hoth
