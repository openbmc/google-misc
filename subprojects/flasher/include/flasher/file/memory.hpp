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
#include <flasher/file.hpp>
#include <stdplus/fd/managed.hpp>

#include <vector>

namespace flasher
{
namespace file
{

class Memory : public File
{
  public:
    size_t getSize() const override;
    void truncate(size_t new_size) override;
    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf,
                                    size_t offset) override;
    stdplus::span<const std::byte> writeAt(stdplus::span<const std::byte> data,
                                           size_t offset) override;

    std::vector<std::byte> data;
};

} // namespace file
} // namespace flasher
