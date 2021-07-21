#pragma once
#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/mutate.hpp>

#include <cstdint>
#include <optional>

namespace flasher
{
namespace ops
{

void automatic(Device& dev, size_t dev_offset, File& file, size_t file_offset,
               Mutate& mutate, size_t max_size,
               std::optional<size_t> stride_size, bool noread);
void read(Device& dev, size_t dev_offset, File& file, size_t file_offset,
          Mutate& mutate, size_t max_size, std::optional<size_t> stride_size);
void write(Device& dev, size_t dev_offset, File& file, size_t file_offset,
           Mutate& mutate, size_t max_size, std::optional<size_t> stride_size,
           bool noread);
void erase(Device& dev, size_t dev_offset, size_t max_size,
           std::optional<size_t> stride_size, bool noread);
void verify(Device& dev, size_t dev_offset, File& file, size_t file_offset,
            Mutate& mutate, size_t max_size, std::optional<size_t> stride_size);

} // namespace ops
} // namespace flasher
