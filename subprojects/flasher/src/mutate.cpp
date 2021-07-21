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

#include <flasher/mutate.hpp>

namespace flasher
{

void NestedMutate::forward(stdplus::span<std::byte> data, size_t offset)
{
    for (auto it = mutations.begin(); it != mutations.end(); ++it)
    {
        (*it)->forward(data, offset);
    }
}

void NestedMutate::reverse(stdplus::span<std::byte> data, size_t offset)
{
    for (auto it = mutations.rbegin(); it != mutations.rend(); ++it)
    {
        (*it)->reverse(data, offset);
    }
}

ModTypeMap<MutateType> mutateTypes;

std::unique_ptr<Mutate> openMutate(const ModArgs& args)
{
    return openMod<Mutate>(mutateTypes, args);
}

} // namespace flasher
