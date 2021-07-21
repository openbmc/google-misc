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

#include <flasher/mutate/rot128.hpp>

namespace flasher
{
namespace mutate
{

void Rot128::forward(stdplus::span<std::byte> data, size_t)
{
    for (auto& c : data)
    {
        c ^= static_cast<std::byte>('\x80');
    }
}

void Rot128::reverse(stdplus::span<std::byte> data, size_t offset)
{
    forward(data, offset);
}

class Rot128Type : public MutateType
{
  public:
    std::unique_ptr<Mutate> open(const ModArgs& args) override
    {
        if (args.arr.size() != 1)
        {
            throw std::invalid_argument("Requires no arguments");
        }
        return std::make_unique<Rot128>();
    }

    void printHelp() const override
    {
        fmt::print(stderr, "  `rot128` mutator (no options)\n");
    }
};

void registerRot128() __attribute__((constructor));
void registerRot128()
{
    mutateTypes.emplace("rot128", std::make_unique<Rot128Type>());
}

void unregisterRot128() __attribute__((destructor));
void unregisterRot128()
{
    mutateTypes.erase("rot128");
}

} // namespace mutate
} // namespace flasher
