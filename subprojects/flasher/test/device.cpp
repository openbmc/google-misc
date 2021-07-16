#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flasher/device.hpp>
#include <flasher/device/mock.hpp>
#include <utility>

namespace flasher
{

TEST(OpenDevice, Missing)
{
    ModArgs args("");
    args.arr.clear();
    EXPECT_THROW(openDevice(args), std::invalid_argument);
}

TEST(OpenDevice, Invalid)
{
    ModArgs args("invalid");
    EXPECT_THROW(openDevice(args), std::invalid_argument);
}

TEST(OpenDevice, Valid)
{
    ModArgs args("mtd,/dev/null");
    EXPECT_THROW(openDevice(args), std::system_error);
}

TEST(Device, Type)
{
  EXPECT_EQ(Device::Type::Nor, Device::toType("nor"));
  EXPECT_EQ(Device::Type::Simple, Device::toType("simple"));
}

TEST(Device, Nor)
{
	EXPECT_THROW(device::Mock(Device::Type::Nor, 4097, 4096), std::invalid_argument);
	EXPECT_THROW(device::Mock(Device::Type::Nor, 8192, 0), std::invalid_argument);

	device::Mock dev(Device::Type::Nor, 8192, 4096);
	EXPECT_EQ(8192, dev.getSize());
	EXPECT_EQ(4096, dev.getEraseSize());

	EXPECT_EQ(0, dev.eraseAlignStart(4095));
	EXPECT_EQ(4096, dev.eraseAlignStart(4096));
	EXPECT_EQ(4096, dev.eraseAlignStart(4097));
	EXPECT_EQ(4096, dev.eraseAlignStart(8191));

	EXPECT_EQ(4096, dev.eraseAlignEnd(15));
	EXPECT_EQ(4096, dev.eraseAlignEnd(4095));
	EXPECT_EQ(4096, dev.eraseAlignEnd(4096));
	EXPECT_EQ(8192, dev.eraseAlignEnd(4097));
	EXPECT_EQ(8192, dev.eraseAlignEnd(8191));

	EXPECT_FALSE(dev.needsErase({}, {}));
	std::byte a[] = {std::byte{0x10}};
	std::byte b[] = {std::byte{0x30}};
	std::byte c[] = {std::byte{0x01}};
	EXPECT_THROW(dev.needsErase({}, a), std::invalid_argument);
	EXPECT_TRUE(dev.needsErase(a, b));
	EXPECT_TRUE(dev.needsErase(c, a));
	EXPECT_TRUE(dev.needsErase(a, c));
	EXPECT_FALSE(dev.needsErase(b, b));
	EXPECT_FALSE(dev.needsErase(b, a));

	dev.mockErase(a);
	EXPECT_EQ(std::byte{0xff}, a[0]);
}

TEST(Device, Simple)
{
	EXPECT_THROW(device::Mock(Device::Type::Simple, 8192, 1), std::invalid_argument);
     device::Mock dev(Device::Type::Simple, 8192, 0);
	EXPECT_EQ(8192, dev.getSize());
	EXPECT_EQ(0, dev.getEraseSize());

	EXPECT_EQ(4095, dev.eraseAlignStart(4095));
	EXPECT_EQ(4096, dev.eraseAlignStart(4096));
	EXPECT_EQ(4097, dev.eraseAlignStart(4097));
	EXPECT_EQ(8191, dev.eraseAlignStart(8191));

	EXPECT_EQ(15, dev.eraseAlignEnd(15));
	EXPECT_EQ(4095, dev.eraseAlignEnd(4095));
	EXPECT_EQ(4096, dev.eraseAlignEnd(4096));
	EXPECT_EQ(4097, dev.eraseAlignEnd(4097));
	EXPECT_EQ(8191, dev.eraseAlignEnd(8191));

	EXPECT_FALSE(dev.needsErase({}, {}));
	std::byte a[] = {std::byte{0x10}};
	std::byte b[] = {std::byte{0x01}};
	EXPECT_FALSE(dev.needsErase(b, b));
	EXPECT_FALSE(dev.needsErase(b, b));
	EXPECT_FALSE(dev.needsErase(a, b));

	dev.mockErase(a);
	EXPECT_EQ(std::byte{0x10}, a[0]);
}

}
