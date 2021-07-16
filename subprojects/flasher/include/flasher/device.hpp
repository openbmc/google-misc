#pragma once
#include <flasher/mod.hpp>
#include <stdplus/types.hpp>

#include <cstddef>
#include <string_view>

namespace flasher
{

class Device
{
  public:
    enum class Type
    {
        Nor,
        Simple,
    };
    static Type toType(std::string_view str);

  protected:
    struct DeviceInfo
    {
        Type type;
        size_t size;
        size_t erase_size;
    };

  public:
    Device(DeviceInfo&& info);
    virtual ~Device() = default;

    virtual stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf,
                                            size_t offset) = 0;
    virtual stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) = 0;
    virtual void eraseBlocks(size_t idx, size_t num) = 0;
    virtual size_t recommendedStride() const;

    void readAtExact(stdplus::span<std::byte> data, size_t offset);
    void writeAtExact(stdplus::span<const std::byte> data, size_t offset);

    size_t getSize() const;
    size_t getEraseSize() const;
    size_t eraseAlignStart(size_t offset) const;
    size_t eraseAlignEnd(size_t offset) const;

    bool needsErase(stdplus::span<const std::byte> flash_data,
                    stdplus::span<const std::byte> new_data) const;
    void mockErase(stdplus::span<std::byte> data) const;

  protected:
    DeviceInfo info;
};

class DeviceType : public ModType<Device>
{
  public:
    virtual std::unique_ptr<Device> open(const ModArgs& args) = 0;
};

extern ModTypeMap<DeviceType> deviceTypes;
std::unique_ptr<Device> openDevice(const ModArgs& args);

} // namespace flasher
