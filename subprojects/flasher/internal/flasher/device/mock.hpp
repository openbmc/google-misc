#pragma once
#include <flasher/device.hpp>
#include <gmock/gmock.h>

namespace flasher
{
namespace device
{

struct Mock : public Device
{
    Mock(Type type, size_t size, size_t erase_size) : Device(DeviceInfo{type, size, erase_size}) { }
    MOCK_METHOD(stdplus::span<std::byte>, readAt, (stdplus::span<std::byte> buf, size_t offset), (override));
    MOCK_METHOD(stdplus::span<const std::byte>,
        writeAt, (stdplus::span<const std::byte> data, size_t offset), (override));
    MOCK_METHOD(void, eraseBlocks, (size_t idx, size_t num), (override));
    MOCK_METHOD(size_t, recommendedStride, (), (const, override));
};

} // namespace device
} // namespace flasher
