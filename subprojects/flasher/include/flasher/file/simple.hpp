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

    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf, size_t offset) override;
    stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) override;

  private:
    stdplus::ManagedFd fd;
    size_t offset;
};

} // namespace file
} // namespace flasher
