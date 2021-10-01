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
#include <libcr51sign/libcr51sign.h>

#include <flashupdate/cr51.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

namespace flashupdate
{
namespace cr51
{

struct Mock : public Cr51
{
    Mock() : Cr51()
    {}

    MOCK_METHOD(bool, validateImage,
                (std::string_view file, uint32_t size, Config::Key keys),
                (override));
    MOCK_METHOD(std::string, imageVersion, (), (override, const));
    MOCK_METHOD(std::vector<struct image_region>, persistentRegions, (),
                (override, const));
    MOCK_METHOD(bool, verify, (bool), (override));
    MOCK_METHOD(std::vector<uint8_t>, descriptorHash, (), (override, const));
    MOCK_METHOD(bool, prodImage, (), (override, const));
};

} // namespace cr51
} // namespace flashupdate
