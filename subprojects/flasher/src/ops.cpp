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

#include <flasher/ops.hpp>

namespace flasher
{
namespace ops
{

void automatic(Device&, size_t, File&, size_t, Mutate&, size_t,
               std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void read(Device&, size_t, File&, size_t, Mutate&, size_t,
          std::optional<size_t>)
{
    throw std::runtime_error("Not implemented");
}

void write(Device&, size_t, File&, size_t, Mutate&, size_t,
           std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void erase(Device&, size_t, size_t, std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void verify(Device&, size_t, File&, size_t, Mutate&, size_t,
            std::optional<size_t>)
{
    throw std::runtime_error("Not implemented");
}

} // namespace ops
} // namespace flasher
