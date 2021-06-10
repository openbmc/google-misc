#include <ec-ipmi-blobs/util.hpp>
#include <function2/function2.hpp>

#include <gtest/gtest.h>

namespace blobs
{
namespace ec
{

struct FakeCancelable : public Cancelable
{
    size_t count = 0;
    void cancel() noexcept override
    {
        count++;
    }
};

TEST(UtilTest, Cancel)
{
    FakeCancelable c;
    EXPECT_EQ(c.count, 0);
    {
        Cancel cancel(&c);
        EXPECT_EQ(c.count, 0);
    }
    EXPECT_EQ(c.count, 1);
}

TEST(UtilTest, AlwaysCallOnceLambda)
{
    size_t ctr = 0;
    {
        auto i = std::make_unique<size_t>(1);
        auto acb = alwaysCallOnce([&, i = std::move(i)]() { ctr += *i; });
        EXPECT_EQ(ctr, 0);
        acb();
        EXPECT_EQ(ctr, 1);
    }
    EXPECT_EQ(ctr, 1);
    {
        auto acb = alwaysCallOnce([&](size_t i) { ctr += i; }, 3);
        EXPECT_EQ(ctr, 1);
    }
    EXPECT_EQ(ctr, 4);
}

TEST(UtilTest, AlwaysCallOnceFunction)
{
    size_t ctr = 0;
    {
        auto acb = alwaysCallOnce(fu2::unique_function<void()>(nullptr));
    }
    {
        auto acb =
            alwaysCallOnce(fu2::unique_function<void()>([&]() { ctr++; }));
        EXPECT_EQ(ctr, 0);
        acb = alwaysCallOnce(fu2::unique_function<void()>([&]() { ctr += 2; }));
        EXPECT_EQ(ctr, 1);
    }
    EXPECT_EQ(ctr, 3);
}

} // namespace ec
} // namespace blobs
