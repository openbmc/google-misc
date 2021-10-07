
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
#include <flashupdate/config.hpp>
#include <flashupdate/flash.hpp>

#include <optional>
#include <string>
#include <utility>

#include <gmock/gmock.h>
namespace flashupdate
{
namespace flash
{

struct MockHelper : public FlashHelper
{
    MockHelper() : FlashHelper()
    {}

    MOCK_METHOD(std::string, readMtdFile, (const std::string&), (override));
};

struct Mock : public Flash
{
    Mock(Config config, bool keepMux) : Flash(config, keepMux)
    {}

    MOCK_METHOD((std::optional<std::pair<std::string, uint32_t>>), getFlash,
                (bool), (override));
    MOCK_METHOD(void, cleanup, (), (override));
};


} // namespace flash
} // namespace flashupdate
