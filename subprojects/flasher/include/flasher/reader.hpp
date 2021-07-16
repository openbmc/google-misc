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
#include <stdplus/types.hpp>

#include <cstddef>

class Reader
{
  public:
    virtual ~Reader() = default;

    virtual size_t getSize() const = 0;
    virtual stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf,
                                            size_t offset) = 0;

    void readAtExact(stdplus::span<std::byte> data, size_t offset);
};
