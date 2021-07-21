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

#include <flasher/device/mutated.hpp>

#include <cstring>
#include <utility>

namespace flasher
{
namespace device
{

Mutated::Mutated(Mutate& mut, Device& dev) :
    Device(DeviceInfo{dev.getInfo()}), mut(&mut), dev(&dev)
{}

stdplus::span<std::byte> Mutated::readAt(stdplus::span<std::byte> buf,
                                         size_t offset)
{
    auto ret = dev->readAt(buf, offset);
    mut->reverse(ret, offset);
    return ret;
}

stdplus::span<const std::byte>
    Mutated::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    if (buf.size() < data.size())
        buf.resize(data.size());
    std::memcpy(buf.data(), data.data(), data.size());
    auto ret = stdplus::span<std::byte>(buf).subspan(0, data.size());
    mut->forward(ret, offset);
    return data.subspan(0, dev->writeAt(ret, offset).size());
}

void Mutated::eraseBlocks(size_t idx, size_t num)
{
    dev->eraseBlocks(idx, num);
}

MutatedOwned::MutatedOwned(std::unique_ptr<Mutate>&& mut,
                           std::unique_ptr<Device>&& dev) :
    Mutated(*mut, *dev),
    mut(std::move(mut)), dev(std::move(dev))
{}

} // namespace device
} // namespace flasher
