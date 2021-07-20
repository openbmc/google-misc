#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flasher/file/memory.hpp>
#include <stdplus/exception.hpp>

namespace flasher
{
namespace file
{

using testing::ElementsAre;

constexpr std::byte operator""_b(unsigned long long t)
{
	return static_cast<std::byte>(t);
}

TEST(FileMemory, Empty)
{
    Memory m;

    std::byte buf[2];
    EXPECT_THROW(m.readAt(buf, 0), stdplus::exception::Eof);
}

TEST(FileMemory, EmptyExpand)
{
    Memory m;
    m.writeAt({}, 1);

    std::byte buf[2];
    EXPECT_THAT(m.readAt(buf, 0), ElementsAre(0_b));
    EXPECT_THROW(m.readAt(buf, 1), stdplus::exception::Eof);
    EXPECT_THROW(m.readAt(buf, 2), std::runtime_error);
}

TEST(FileMemory, WithData)
{
    Memory m;
    m.data = {1_b, 2_b, 3_b, 4_b, 0_b};

    std::byte buf[2];
    EXPECT_THAT(m.readAt(buf, 1), ElementsAre(2_b, 3_b));
    EXPECT_THAT(m.readAt(buf, 3), ElementsAre(4_b, 0_b));
    EXPECT_THROW(m.readAt(buf, 5), stdplus::exception::Eof);

    EXPECT_EQ(m.writeAt(std::vector{10_b, 11_b}, 2).size(), 2);
    EXPECT_THAT(m.readAt(buf, 2), ElementsAre(10_b, 11_b));
    EXPECT_EQ(m.writeAt(std::vector{12_b, 13_b}, 4).size(), 2);
    EXPECT_EQ(m.data.size(), 6);
    EXPECT_THAT(m.readAt(buf, 4), ElementsAre(12_b, 13_b));
}

}
}
