#include <flasher/convert.hpp>

#include <gtest/gtest.h>

namespace flasher
{

TEST(Convert, Uint32)
{
    EXPECT_EQ(toUint32("0x40"), 64);
    EXPECT_EQ(toUint32("40"), 40);
    EXPECT_EQ(toUint32("0"), 0);

    EXPECT_THROW(toUint32(""), std::runtime_error);
    EXPECT_THROW(toUint32(","), std::runtime_error);
    EXPECT_THROW(toUint32(",0"), std::runtime_error);
    EXPECT_THROW(toUint32("0,"), std::runtime_error);
}

} // namespace flasher
