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
#include <flasher/mod.hpp>
#include <flasher/reader.hpp>
#include <stdplus/types.hpp>

#include <cstddef>
#include <string_view>

namespace flasher
{

class Device : public Reader
{
  public:
    enum class Type
    {
        Nor,
        Simple,
    };
    static Type toType(std::string_view str);

  protected:
    struct DeviceInfo
    {
        Type type;
        size_t size;
        size_t erase_size;
    };

  public:
    Device(DeviceInfo&& info);

    virtual stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) = 0;
    virtual void eraseBlocks(size_t idx, size_t num) = 0;
    virtual size_t recommendedStride() const;

    void writeAtExact(stdplus::span<const std::byte> data, size_t offset);

    inline const DeviceInfo& getInfo() const
    {
        return info;
    }
    size_t getSize() const override;
    inline size_t getEraseSize() const
    {
        return info.erase_size;
    }

    size_t eraseAlignStart(size_t offset) const;
    size_t eraseAlignEnd(size_t offset) const;

    bool needsErase(stdplus::span<const std::byte> flash_data,
                    stdplus::span<const std::byte> new_data) const;
    void mockErase(stdplus::span<std::byte> data) const;

  protected:
    DeviceInfo info;
};

class DeviceType : public ModType<Device>
{
  public:
    virtual std::unique_ptr<Device> open(const ModArgs& args) = 0;
};

extern ModTypeMap<DeviceType> deviceTypes;
std::unique_ptr<Device> openDevice(const ModArgs& args);

} // namespace flasher
