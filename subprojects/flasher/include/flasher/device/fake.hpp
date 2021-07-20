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
#include <flasher/file.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace flasher
{
namespace device
{

class Fake : public Device
{
  public:
    Fake(File& file, Type type, size_t erase);

    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf,
                                    size_t offset) override;
    stdplus::span<const std::byte> writeAt(stdplus::span<const std::byte> data,
                                           size_t offset) override;
    void eraseBlocks(size_t idx, size_t num) override;

  private:
    File* file;
    std::vector<std::byte> erase_contents;

    static DeviceInfo buildDeviceInfo(File& file, Type type, size_t erase);
};

class FakeOwning : public Fake
{
  public:
    FakeOwning(std::unique_ptr<File>&& file, Type type, size_t erase);

  private:
    std::unique_ptr<File> file;
};

} // namespace device
} // namespace flasher
