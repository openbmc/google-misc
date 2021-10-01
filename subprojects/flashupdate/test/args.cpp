#include <flashupdate/args.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace flashupdate
{

using flasher::ModArgs;

class ArgsTest : public ::testing::Test
{
  protected:
    ArgsTest()
    {}

    Args vecArgs(std::vector<std::string> args)
    {
        std::vector<char*> argv;
        for (auto& arg : args)
            argv.push_back(arg.data());

        argv.push_back(nullptr);
        return Args(args.size(), argv.data());
    }
};

TEST_F(ArgsTest, OpRequired)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "-v"}), std::runtime_error);
}

TEST_F(ArgsTest, InjectPersistentTest)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "inject_persistent"}),
                 std::runtime_error);
    auto args = vecArgs({"flasheupdate", "inject_persistent", "file"});

    EXPECT_EQ(args.op, Args::Op::InjectPersistent);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, HashDescriptor)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "hash_descriptor"}),
                 std::runtime_error);
    auto args = vecArgs({"flasheupdate", "hash_descriptor", "file"});

    EXPECT_EQ(args.op, Args::Op::HashDescriptor);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, ReadTest)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "read"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasheupdate", "read", "primary"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasheupdate", "read", "other", "file"}),
                 std::runtime_error);

    auto args = vecArgs({"flasheupdate", "read", "primary", "file"});
    EXPECT_EQ(args.op, Args::Op::Read);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, true);
    EXPECT_EQ(args.stagingIndex, 0);

    args = vecArgs({"flasheupdate", "read", "secondary", "file"});
    EXPECT_EQ(args.op, Args::Op::Read);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, false);
    EXPECT_EQ(args.stagingIndex, 0);
}

TEST_F(ArgsTest, WriteTest)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "write"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasheupdate", "write", "file"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasheupdate", "write", "file", "other"}),
                 std::runtime_error);

    auto args = vecArgs({"flasheupdate", "write", "file", "primary"});
    EXPECT_EQ(args.op, Args::Op::Write);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, true);
    EXPECT_EQ(args.stagingIndex, 0);

    args = vecArgs({"flasheupdate", "write", "file", "secondary"});
    EXPECT_EQ(args.op, Args::Op::Write);
    EXPECT_EQ(args.file, ModArgs("file"));
    EXPECT_EQ(args.primary, false);
    EXPECT_EQ(args.stagingIndex, 0);
}

TEST_F(ArgsTest, UpdateStateTest)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "update_state"}), std::runtime_error);

    auto args = vecArgs({"flasheupdate", "update_state", "state"});
    EXPECT_EQ(args.op, Args::Op::UpdateState);
    EXPECT_EQ(args.file, std::nullopt);
    EXPECT_EQ(args.state, "state");
}

TEST_F(ArgsTest, UpdateStagedVersionTest)
{
    EXPECT_THROW(vecArgs({"flasheupdate", "update_staged_version"}),
                 std::runtime_error);

    auto args = vecArgs({"flasheupdate", "update_staged_version", "file"});
    EXPECT_EQ(args.op, Args::Op::UpdateStagedVersion);
    EXPECT_EQ(args.file, ModArgs("file"));
}

TEST_F(ArgsTest, Verbose)
{
    EXPECT_EQ(0, vecArgs({"flasheupdate", "empty"}).verbose);
    EXPECT_EQ(
        4,
        vecArgs({"flasheupdate", "--verbose", "-v", "empty", "-vv"}).verbose);
}

} // namespace flashupdate
