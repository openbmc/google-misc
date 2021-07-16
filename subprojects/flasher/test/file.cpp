#include <gtest/gtest.h>
#include <flasher/file.hpp>

namespace flasher
{

using stdplus::fd::OpenFlags;
using stdplus::fd::OpenAccess;

TEST(OpenFile, Missing)
{
    ModArgs args("");
    args.arr.clear();
    EXPECT_THROW(openFile(args, OpenFlags(OpenAccess::ReadOnly)), std::invalid_argument);
}

TEST(OpenFile, Invalid)
{
    ModArgs args("invalid,/dev/null");
    EXPECT_THROW(openFile(args, OpenFlags(OpenAccess::ReadOnly)), std::invalid_argument);
}

TEST(OpenDevice, SimpleDefault)
{
    ModArgs args("/dev/null");
    openFile(args, OpenFlags(OpenAccess::ReadOnly));
}

TEST(OpenDevice, Valid)
{
    ModArgs args("simple,/dev/null");
    openFile(args, OpenFlags(OpenAccess::ReadOnly));
}

}
