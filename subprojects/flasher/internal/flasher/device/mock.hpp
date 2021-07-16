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
#include <flasher/device.hpp>

#include <gmock/gmock.h>

namespace flasher
{
namespace device
{

struct Mock : public Device
{
    Mock(Type type, size_t size, size_t erase_size) :
        Device(DeviceInfo{type, size, erase_size})
    {}
    MOCK_METHOD(stdplus::span<std::byte>, readAt,
                (stdplus::span<std::byte> buf, size_t offset), (override));
    MOCK_METHOD(stdplus::span<const std::byte>, writeAt,
                (stdplus::span<const std::byte> data, size_t offset),
                (override));
    MOCK_METHOD(void, eraseBlocks, (size_t idx, size_t num), (override));
    MOCK_METHOD(size_t, recommendedStride, (), (const, override));
};

} // namespace device
} // namespace flasher
