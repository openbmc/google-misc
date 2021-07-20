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

#include <flasher/file/memory.hpp>
#include <stdplus/exception.hpp>

#include <algorithm>
#include <cstring>

namespace flasher
{
namespace file
{

size_t Memory::getSize() const
{
    return data.size();
}

void Memory::truncate(size_t new_size)
{
    data.resize(new_size);
}

stdplus::span<std::byte> Memory::readAt(stdplus::span<std::byte> buf,
                                        size_t offset)
{
    if (offset == data.size())
        throw stdplus::exception::Eof("readAt");
    if (offset > data.size())
        throw std::runtime_error("Read offset out of range");
    auto ret = buf.subspan(0, std::min(buf.size(), data.size() - offset));
    std::memcpy(ret.data(), &data[offset], ret.size());
    return ret;
}

stdplus::span<const std::byte>
    Memory::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    this->data.resize(std::max(this->data.size(), offset + data.size()));
    std::memcpy(&this->data[offset], data.data(), data.size());
    return stdplus::span<const std::byte>(this->data)
        .subspan(offset, data.size());
}

} // namespace file
} // namespace flasher
