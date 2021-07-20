#pragma once
#include <flasher/file.hpp>
#include <stdplus/fd/managed.hpp>

#include <vector>

namespace flasher
{
namespace file
{

class Memory : public File
{
  public:
    size_t getSize() const override;
    void truncate(size_t new_size) override;
    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf, size_t offset) override;
    stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) override;

    std::vector<std::byte> data;
};

} // namespace file
} // namespace flasher
