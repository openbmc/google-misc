#pragma once
#include <flasher/file.hpp>
#include <flasher/device.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace flasher
{
namespace device
{

class Fake : public Device
{
  public:
    Fake(File &file, Type type, size_t erase);

    stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf, size_t offset) override;
    stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) override;
    void eraseBlocks(size_t idx, size_t num) override;

  private:
    File *file;
    std::vector<std::byte> erase_contents;

    static DeviceInfo buildDeviceInfo(File& file, Type type, size_t erase);
};

class FakeOwning : public Fake
{
  public:
    FakeOwning(std::unique_ptr<File>&& file, Type type, size_t erase);

  private:
    std::unique_ptr<File> file;
};

} // namespace device
} // namespace flasher
