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
#include <stdplus/types.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace flasher
{

class Mutate
{
  public:
    virtual ~Mutate() = default;
    virtual void forward(stdplus::span<std::byte> data, size_t offset) = 0;
    virtual void reverse(stdplus::span<std::byte> data, size_t offset) = 0;
};

class NestedMutate : public Mutate
{
  public:
    std::vector<std::unique_ptr<Mutate>> mutations;

    void forward(stdplus::span<std::byte> data, size_t offset);
    void reverse(stdplus::span<std::byte> data, size_t offset);
};

class MutateType : public ModType<Mutate>
{
  public:
    virtual std::unique_ptr<Mutate> open(const ModArgs& args) = 0;
};

extern ModTypeMap<MutateType> mutateTypes;
std::unique_ptr<Mutate> openMutate(const ModArgs& args);

} // namespace flasher
