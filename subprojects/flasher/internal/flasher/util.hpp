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
#include <fmt/format.h>

#include <stdplus/exception.hpp>
#include <stdplus/fd/intf.hpp>
#include <stdplus/types.hpp>

#include <utility>

namespace flasher
{

template <typename Func, typename Fd, typename Byte, typename... Args>
static void opAtExact(const char* name, Func&& func, Fd& fd,
                      stdplus::span<Byte> data, size_t offset, Args&&... args)
{
    while (data.size() > 0)
    {
        auto ret = (fd.*func)(data, offset, std::forward<Args>(args)...);
        if (ret.size() == 0)
        {
            throw stdplus::exception::WouldBlock(
                fmt::format("{} missing {}B", name, data.size()));
        }
        offset += ret.size();
        data = data.subspan(ret.size());
    }
}

template <typename Func, typename Fd, typename Byte, typename... Args>
static stdplus::span<Byte> opAt(Func&& func, Fd& fd, size_t& cur_offset,
                                stdplus::span<Byte> data, size_t new_offset,
                                Args&&... args)
{
    if (cur_offset != new_offset)
    {
        fd.lseek(new_offset, stdplus::fd::Whence::Set);
        cur_offset = new_offset;
    }
    auto ret = (fd.*func)(data, std::forward<Args>(args)...);
    cur_offset += ret.size();
    return ret;
}

} // namespace flasher
