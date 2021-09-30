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

#include "cr51sign_validator.h"

#include <optional>
#include <span>
#include <vector>

#include <gmock/gmock.h>

namespace google
{
namespace cr51
{

struct Mock : public Cr51SignValidator
{
    Mock() : Cr51SignValidator(){};

    MOCK_METHOD(std::span<const uint8_t>, getBuffer, (), (override));
    MOCK_METHOD(bool, readImage, (std::span<std::byte> file, size_t size),
                (override));
    MOCK_METHOD(std::span<const uint8_t>, hashDescriptor,
                (struct libcr51sign_ctx * ctx), (override));
    MOCK_METHOD(std::optional<struct libcr51sign_validated_regions>,
                validateDescriptor, (struct libcr51sign_ctx * ctx), (override));
};

} // namespace cr51
} // namespace google