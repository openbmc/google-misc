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
#include <optional>

namespace flasher
{
namespace ops
{

void automatic(Device& dev, size_t dev_offset, File& file, size_t file_offset,
               size_t max_size, std::optional<size_t> stride_size, bool noread);
void read(Device& dev, size_t dev_offset, File& file, size_t file_offset,
          size_t max_size, std::optional<size_t> stride_size);
void write(Device& dev, size_t dev_offset, File& file, size_t file_offset,
           size_t max_size, std::optional<size_t> stride_size, bool noread);
void erase(Device& dev, size_t dev_offset, size_t max_size,
           std::optional<size_t> stride_size, bool noread);
void verify(Device& dev, size_t dev_offset, File& file, size_t file_offset,
            size_t max_size, std::optional<size_t> stride_size);

} // namespace ops
} // namespace flasher
