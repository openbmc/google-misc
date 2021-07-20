#include <gtest/gtest.h>
#include <memory>
#include <flasher/file/memory.hpp>
#include <flasher/device.hpp>
#include <flasher/device/fake.hpp>
#include <sys/mman.h>
#include <gmock/gmock.h>
#include <utility>

namespace flasher
{
namespace device
{

using testing::ElementsAre;

constexpr std::byte operator""_b(unsigned long long t)
{
	return static_cast<std::byte>(t);
}

TEST(FakeSimpleDevice, EraseWrite)
{
    file::Memory m;

    Fake f(m, Fake::Type::Simple, 0);
    EXPECT_EQ(f.getSize(), 0);
    EXPECT_EQ(f.getEraseSize(), 0);

    m.data.resize(8);
    f = Fake(m, Fake::Type::Simple, 0);
    EXPECT_EQ(f.getSize(), 8);
    EXPECT_EQ(f.getEraseSize(), 0);
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b}, 0).size(), 2);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0xf0_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b, 0xff_b, 0x00_b}, 1).size(), 4);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x0f_b, 0xf0_b, 0xff_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b));
    f.eraseBlocks(1, 1);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x0f_b, 0xf0_b, 0xff_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b));
}

TEST(FakeNorDevice, EraseWrite)
{
    file::Memory m;

    Fake f(m, Fake::Type::Nor, 8);
    EXPECT_EQ(f.getSize(), 0);
    EXPECT_EQ(f.getEraseSize(), 8);

    m.data.resize(8);
    f = Fake(m, Fake::Type::Nor, 4);
    EXPECT_EQ(f.getSize(), 8);
    EXPECT_EQ(f.getEraseSize(), 4);
    f.eraseBlocks(0, 2);
    EXPECT_THAT(m.data, ElementsAre(0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b}, 0).size(), 2);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0xf0_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b));
    EXPECT_EQ(f.writeAt(std::vector{0x0f_b, 0xf0_b, 0xff_b, 0x00_b}, 1).size(), 4);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x00_b, 0xf0_b, 0xff_b, 0x00_b, 0xff_b, 0xff_b, 0xff_b));
    f.eraseBlocks(1, 1);
    EXPECT_THAT(m.data, ElementsAre(0x0f_b, 0x00_b, 0xf0_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b));
}

TEST(OpenFakeDevice, RequiredArguments)
{
  EXPECT_THROW(openDevice(ModArgs("fake,/dev/null")), std::invalid_argument);
  EXPECT_THROW(openDevice(ModArgs("fake,type=nor,erase=4096")), std::invalid_argument);
  EXPECT_THROW(openDevice(ModArgs("fake,type=nor,/dev/null")), std::invalid_argument);
  EXPECT_THROW(openDevice(ModArgs("fake,erase=4096,/dev/null")), std::invalid_argument);
}

TEST(OpenFakeDevice, OpenFake)
{
  auto dev = openDevice(ModArgs("fake,type=nor,erase=4096,/dev/null"));
  EXPECT_EQ(dev->getSize(), 0);
}

}
}
