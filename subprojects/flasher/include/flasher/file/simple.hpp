#pragma once
#include <flasher/file.hpp>
#include <stdplus/fd/managed.hpp>

namespace flasher
{
namespace file
{

class Simple : public File
{
  public:
    Simple(stdplus::ManagedFd&& fd);

    size_t getSize() const override;
    void truncate(size_t new_size) override;
    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf, size_t offset) override;
    stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) override;

  private:
    stdplus::ManagedFd fd;
    size_t size, offset;
};

} // namespace file
} // namespace flasher
