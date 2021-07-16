#include <flasher/mod.hpp>

#include <gtest/gtest.h>

namespace flasher
{

TEST(ModArgs, Empty)
{
    ModArgs args("");
    EXPECT_TRUE(args.dict.empty());
    EXPECT_EQ(args.arr, std::vector<std::string>{""});
}

TEST(ModArgs, Simple)
{
    ModArgs args("abcd");
    EXPECT_TRUE(args.dict.empty());
    EXPECT_EQ(args.arr, std::vector<std::string>{"abcd"});
}

TEST(ModArgs, MultiArgs)
{
    ModArgs args(",a1,a=b,a\\=b,c\\,d,hi=/no-such-path,");
    EXPECT_EQ(args.dict, (std::unordered_map<std::string, std::string>{
                             {"a", "b"}, {"hi", "/no-such-path"}}));
    EXPECT_EQ(args.arr, (std::vector<std::string>{"", "a1", "a=b", "c,d", ""}));
}

} // namespace flasher
