#pragma once
#include <flasher/device.hpp>
#include <stdplus/fd/intf.hpp>
#include <stdplus/fd/managed.hpp>

namespace flasher
{
namespace device
{

class Mtd : public Device
{
  public:
    Mtd(stdplus::ManagedFd&& fd);

    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf, size_t offset) override;
    stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) override;
    void eraseBlocks(size_t idx, size_t num) override;

  private:
    stdplus::ManagedFd fd;
    size_t offset;

    static DeviceInfo buildDeviceInfo(stdplus::Fd& fd);
};

} // namespace device
} // namespace flasher
