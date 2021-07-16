// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <flasher/args.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace flasher
{

Args vecArgs(std::vector<std::string> args)
{
    std::vector<char*> argv;
    for (auto& arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);
    return Args(args.size(), argv.data());
}

TEST(ArgsTest, OpRequired)
{
    EXPECT_THROW(vecArgs({"flasher", "-v"}), std::runtime_error);
}

TEST(ArgsTest, AutoWorks)
{
    EXPECT_THROW(vecArgs({"flasher", "auto", "file"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "auto", "file", "dev", "noop"}),
                 std::runtime_error);
    auto args = vecArgs({"flasher", "auto", "file", "dev"});
    EXPECT_EQ(Args::Op::Automatic, args.op);
    EXPECT_EQ(ModArgs("file"), args.file);
    EXPECT_EQ(ModArgs("dev"), args.dev);
}

TEST(ArgsTest, WriteWorks)
{
    EXPECT_THROW(vecArgs({"flasher", "write", "file"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "write", "file", "dev", "noop"}),
                 std::runtime_error);
    auto args = vecArgs({"flasher", "write", "file", "dev"});
    EXPECT_EQ(Args::Op::Write, args.op);
    EXPECT_EQ(ModArgs("file"), args.file);
    EXPECT_EQ(ModArgs("dev"), args.dev);
}

TEST(ArgsTest, VerifyWorks)
{
    EXPECT_THROW(vecArgs({"flasher", "verify", "file"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "verify", "file", "dev", "noop"}),
                 std::runtime_error);
    auto args = vecArgs({"flasher", "verify", "file", "dev"});
    EXPECT_EQ(Args::Op::Verify, args.op);
    EXPECT_EQ(ModArgs("file"), args.file);
    EXPECT_EQ(ModArgs("dev"), args.dev);
}

TEST(ArgsTest, ReadWorks)
{
    EXPECT_THROW(vecArgs({"flasher", "read", "dev"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "read", "dev", "file", "noop"}),
                 std::runtime_error);
    auto args = vecArgs({"flasher", "read", "dev", "file"});
    EXPECT_EQ(Args::Op::Read, args.op);
    EXPECT_EQ(ModArgs("file"), args.file);
    EXPECT_EQ(ModArgs("dev"), args.dev);
}

TEST(ArgsTest, EraseWorks)
{
    EXPECT_THROW(vecArgs({"flasher", "erase"}), std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "noop"}),
                 std::runtime_error);
    auto args = vecArgs({"flasher", "erase", "dev"});
    EXPECT_EQ(Args::Op::Erase, args.op);
    EXPECT_EQ(std::nullopt, args.file);
    EXPECT_EQ(ModArgs("dev"), args.dev);
}

TEST(ArgsTest, Offset)
{
    EXPECT_EQ(0, vecArgs({"flasher", "erase", "dev"}).dev_offset);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-o"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-o", "10a"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-o", "c10"}),
                 std::runtime_error);
    EXPECT_EQ(
        17,
        vecArgs({"flasher", "erase", "dev", "--dev-offset", "17"}).dev_offset);
    EXPECT_EQ(16,
              vecArgs({"flasher", "erase", "-o", "0x10", "dev"}).dev_offset);
}

TEST(ArgsTest, MaxSize)
{
    EXPECT_EQ(std::numeric_limits<size_t>::max(),
              vecArgs({"flasher", "erase", "dev"}).max_size);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-s"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-s", "10a"}),
                 std::runtime_error);
    EXPECT_THROW(vecArgs({"flasher", "erase", "dev", "-s", "c10"}),
                 std::runtime_error);
    EXPECT_EQ(17,
              vecArgs({"flasher", "erase", "dev", "--size", "17"}).max_size);
    EXPECT_EQ(16, vecArgs({"flasher", "erase", "-s", "0x10", "dev"}).max_size);
}

TEST(ArgsTest, Verbose)
{
    EXPECT_EQ(0, vecArgs({"flasher", "erase", "dev"}).verbose);
    EXPECT_EQ(
        4,
        vecArgs({"flasher", "--verbose", "-v", "erase", "dev", "-vv"}).verbose);
}

} // namespace flasher
