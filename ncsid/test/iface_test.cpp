#include "net_iface_mock.h"

#include <gtest/gtest.h>

TEST(TestIFace, TestGetIndex)
{
    mock::IFace iface_mock;

    constexpr int test_index = 5;
    iface_mock.index = test_index;

    EXPECT_EQ(test_index, iface_mock.get_index());
}

TEST(TestIFace, TestSetClearFlags)
{
    mock::IFace iface_mock;

    const short new_flags = 0xab;
    iface_mock.set_sock_flags(0, new_flags);
    EXPECT_EQ(new_flags, new_flags & iface_mock.flags);
    iface_mock.clear_sock_flags(0, 0xa0);
    EXPECT_EQ(0xb, new_flags & iface_mock.flags);
}
