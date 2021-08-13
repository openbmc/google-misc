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

#include <flasher/logging.hpp>
#include <flasher/ops.hpp>
#include <stdplus/exception.hpp>

#include <algorithm>

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

void erase(Device& dev, size_t dev_offset, size_t max_size,
           std::optional<size_t> stride_size, bool noread)
{
    const auto erase_size = dev.getEraseSize();
    if (erase_size == 0)
    {
        throw std::invalid_argument("Cannot erase device with erase_size 0");
    }
    if (dev_offset % erase_size != 0)
    {
        throw std::invalid_argument(
            fmt::format("Device offset not divisible by erase, {} % {} != 0",
                        dev_offset, erase_size));
    }
    size_t stride = stride_size ? *stride_size : dev.recommendedStride();
    if (stride == 0)
    {
        throw std::invalid_argument("Stride cannot be 0");
    }
    if (stride % erase_size != 0)
    {
        throw std::invalid_argument(fmt::format(
            "Stride not divisible by erase, {} % {} != 0", stride, erase_size));
    }
    if (dev_offset > dev.getSize())
    {
        throw std::invalid_argument(fmt::format(
            "Device smaller than offset, {} < {}", dev.getSize(), dev_offset));
    }
    max_size = std::min(max_size, dev.getSize() - dev_offset);
    if (max_size % erase_size != 0)
    {
        throw std::invalid_argument(fmt::format(
            "Size not divisible by erase, {} % {} != 0", max_size, erase_size));
    }
    std::vector<std::byte> buf_v(stride), erased_v(stride);
    dev.mockErase(erased_v);
    const stdplus::span<std::byte> buf(buf_v), erased(erased_v);
    for (size_t i = dev_offset; i < max_size + dev_offset; i += stride)
    {
        const size_t bytes = std::min(stride, max_size + dev_offset - i);
        if (!noread)
        {
            dev.readAtExact(buf.subspan(0, bytes), i);
            log(LogLevel::Info, " RD@{}#{}", i, bytes);
            if (!dev.needsErase(buf.subspan(0, bytes),
                                erased.subspan(0, bytes)))
            {
                continue;
            }
        }
        dev.eraseBlocks(i / erase_size, bytes / erase_size);
        log(LogLevel::Info, " ED@{}#{}", i, bytes);
    }
    log(LogLevel::Info, "\n");
}

void verify(Device& dev, size_t dev_offset, File& file, size_t file_offset,
            Mutate& mutate, size_t max_size, std::optional<size_t> stride_size)
{
    if (dev_offset > dev.getSize())
    {
        throw std::invalid_argument(fmt::format(
            "Device smaller than offset, {} < {}", dev.getSize(), dev_offset));
    }
    if (stride_size && *stride_size == 0)
    {
        throw std::invalid_argument("Stride cannot be 0");
    }
    size_t stride = stride_size ? *stride_size : dev.recommendedStride();
    std::vector<std::byte> file_buf_v(stride * 2), dev_buf_v(stride * 2);
    const stdplus::span<std::byte> file_buf(file_buf_v), dev_buf(dev_buf_v);
    stdplus::span<std::byte> file_data, dev_data;
    bool eof = false;
    while (max_size > 0)
    {
        if (file_data.size() < stride && max_size > file_data.size() && !eof)
        {
            try
            {
                auto new_file_data = file.readAt(
                    file_buf.subspan(
                        file_data.size(),
                        std::min(stride, max_size - file_data.size())),
                    file_offset);
                log(LogLevel::Info, " VF@{}#{}", dev_offset,
                    new_file_data.size());
                file_data = file_buf.subspan(0, file_data.size() +
                                                    new_file_data.size());
                file_offset += new_file_data.size();
            }
            catch (const stdplus::exception::Eof&)
            {
                eof = true;
                max_size = file_data.size();
            }
        }
        if (dev_data.size() < stride && max_size > dev_data.size())
        {
            auto new_size = std::min(
                stride, std::min(max_size, dev.getSize() - dev_offset) -
                            dev_data.size());
            if (new_size == 0)
            {
                throw std::runtime_error(
                    "Device not large enough for verification file");
            }
            auto new_dev_data = dev.readAt(
                dev_buf.subspan(dev_data.size(), new_size), dev_offset);
            log(LogLevel::Info, " VD@{}#{}", dev_offset, new_dev_data.size());
            mutate.reverse(new_dev_data, dev_offset);
            dev_data =
                dev_buf.subspan(0, dev_data.size() + new_dev_data.size());
            dev_offset += new_dev_data.size();
        }
        auto compared = std::min(dev_data.size(), file_data.size());
        if (std::memcmp(dev_data.data(), file_data.data(), compared))
        {
            log(LogLevel::Info, "\n");
            throw std::runtime_error(fmt::format(
                "Bytes #{} D@{} != F@{}", compared,
                dev_offset - dev_data.size(), file_offset - file_data.size()));
        }
        std::memmove(file_data.data(), file_data.data() + compared,
                     file_data.size() - compared);
        file_data = file_data.subspan(0, file_data.size() - compared);
        std::memmove(dev_data.data(), dev_data.data() + compared,
                     dev_data.size() - compared);
        dev_data = dev_data.subspan(0, dev_data.size() - compared);
        max_size -= compared;
    }
    log(LogLevel::Info, "\n");
}

} // namespace ops
} // namespace flasher
