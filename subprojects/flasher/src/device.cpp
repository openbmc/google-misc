#include <fmt/format.h>

#include <flasher/util.hpp>
#include <flasher/device.hpp>

#include <cstring>
#include <stdexcept>
#include <utility>

namespace flasher
{

void Device::readAtExact(stdplus::span<std::byte> data, size_t offset)
{
    opAtExact("Device readAtExact", &Device::readAt, *this, data, offset);
}
void Device::writeAtExact(stdplus::span<const std::byte> data, size_t offset)
{
    opAtExact("Device writeAtExact", &Device::writeAt, *this, data, offset);
}

Device::Type Device::toType(std::string_view str)
{
    if (str == "nor")
    {
        return Type::Nor;
    }
    if (str == "simple")
    {
        return Type::Simple;
    }
    throw std::invalid_argument(fmt::format("Not a device type: {}", str));
}

Device::Device(DeviceInfo&& info) : info(std::move(info))
{
    if (info.erase_size != 0 && info.size % info.erase_size != 0)
    {
        throw std::invalid_argument(
            fmt::format("Flash size {} is not divisible by erase size {}",
                        info.size, info.erase_size));
    }
    switch (info.type) {
        case Type::Nor:
		if (info.erase_size == 0) {
	    throw std::invalid_argument("Nor flashes can't have 0 erase size");

	   }
		break;
	case Type::Simple:
		if (info.erase_size != 0) {
	    throw std::invalid_argument("Simple flashes can't erase");

	   }
		break;
    }
}

size_t Device::recommendedStride() const
{
    return info.erase_size == 0 ? 8192 : info.erase_size;
}

size_t Device::getSize() const
{
    return info.size;
}

size_t Device::getEraseSize() const
{
    return info.erase_size;
}

size_t Device::eraseAlignStart(size_t offset) const
{
    if (info.erase_size == 0) return offset;
    return offset / info.erase_size * info.erase_size;
}

size_t Device::eraseAlignEnd(size_t offset) const
{
    if (info.erase_size == 0) return offset;
    return eraseAlignStart(offset + info.erase_size - 1);
}

bool Device::needsErase(stdplus::span<const std::byte> flash_data,
                        stdplus::span<const std::byte> new_data) const
{
    if (new_data.size() > flash_data.size())
    {
        throw std::invalid_argument("New data bigger than MTD data");
    }
    switch (info.type)
    {
        case Type::Nor:
            for (size_t i = 0; i < new_data.size(); ++i)
            {
                if ((flash_data[i] & new_data[i]) != new_data[i])
                {
                    return true;
                }
            }
            return false;
	case Type::Simple:
	    return false;
    }
    throw std::invalid_argument("Don't know how to erase Device");
}

void Device::mockErase(stdplus::span<std::byte> data) const
{
    switch (info.type)
    {
        case Type::Nor:
            memset(data.data(), 0xff, data.size());
            return;
	case Type::Simple:
	    return;
    }
    throw std::invalid_argument("Don't know how to erase Device");
}

ModTypeMap<DeviceType> deviceTypes;
std::unique_ptr<Device> openDevice(const ModArgs& args)
{
    return openMod<Device>(deviceTypes, args);
}

} // namespace flasher
