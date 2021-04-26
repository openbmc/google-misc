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

#include "util.hpp"

#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"

TEST(IsNumericPath, invalidNumericPath)
{
    std::string badPath{"badNumericPath"};
    int id = -1;
    EXPECT_FALSE(metric_blob::isNumericPath(badPath, id));
    EXPECT_EQ(id, -1);
}

TEST(IsNumericPath, validNumericPath)
{
    std::string goodPath{"proc/10000"};
    int id = -1;
    EXPECT_TRUE(metric_blob::isNumericPath(goodPath, id));
    EXPECT_EQ(id, 10000);
}

TEST(ReadFileThenGrepIntoString, goodFile)
{
    const std::string& fileName = "./test_file";
    std::ofstream ofs(fileName, std::ios::trunc);
    std::string_view content = "This is\ntest\tcontentt\n\n\n\n.\n\n##$#$";
    ofs << content;
    ofs.close();
    std::string readContent = metric_blob::readFileThenGrepIntoString(fileName);
    std::filesystem::remove(fileName);
    EXPECT_EQ(readContent, content);
}

TEST(ReadFileThenGrepIntoString, inexistentFile)
{
    const std::string& fileName = "./inexistent_file";
    std::string readContent = metric_blob::readFileThenGrepIntoString(fileName);
    EXPECT_EQ(readContent, "");
}

TEST(GetTcommUtimeStime, validInput)
{
    // ticks_per_sec is usually 100 on the BMC
    const long ticksPerSec = 100;

    const std::string_view content = "2596 (dbus-broker) R 2577 2577 2577 0 -1 "
                                     "4194560 299 0 1 0 333037 246110 0 0 20 0 "
                                     "1 0 1545 3411968 530 4294967295 65536 "
                                     "246512 2930531712 0 0 0 81923 4";

    metric_blob::TcommUtimeStime t =
        metric_blob::parseTcommUtimeStimeString(content, ticksPerSec);
    const float EPS = 0.01; // The difference was 0.000117188
    EXPECT_LT(std::abs(t.utime - 3330.37), EPS);
    EXPECT_LT(std::abs(t.stime - 2461.10), EPS);
    EXPECT_EQ(t.tcomm, "(dbus-broker)");
}

TEST(GetTcommUtimeStime, invalidInput)
{
    // ticks_per_sec is usually 100 on the BMC
    const long ticksPerSec = 100;

    const std::string_view content =
        "x invalid x x x x x x x x x x x x x x x x x x x x x x x x x x x";

    metric_blob::TcommUtimeStime t =
        metric_blob::parseTcommUtimeStimeString(content, ticksPerSec);

    EXPECT_EQ(t.utime, 0);
    EXPECT_EQ(t.stime, 0);
    EXPECT_EQ(t.tcomm, "invalid");
}

TEST(ParseMeminfoValue, validInput)
{
    const std::string_view content = "MemTotal:        1027040 kB\n"
                                     "MemFree:          868144 kB\n"
                                     "MemAvailable:     919308 kB\n"
                                     "Buffers:           13008 kB\n"
                                     "Cached:            82840 kB\n"
                                     "SwapCached:            0 kB\n"
                                     "Active:            62076 kB\n";
    int value;
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "MemTotal:", value));
    EXPECT_EQ(value, 1027040);
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "MemFree:", value));
    EXPECT_EQ(value, 868144);
    EXPECT_TRUE(
        metric_blob::parseMeminfoValue(content, "MemAvailable:", value));
    EXPECT_EQ(value, 919308);
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "Buffers:", value));
    EXPECT_EQ(value, 13008);
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "Cached:", value));
    EXPECT_EQ(value, 82840);
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "SwapCached:", value));
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(metric_blob::parseMeminfoValue(content, "Active:", value));
    EXPECT_EQ(value, 62076);
}

TEST(ParseMeminfoValue, invalidInput)
{
    const std::string_view invalid = "MemTotal: 1";
    int value = -999;
    EXPECT_FALSE(metric_blob::parseMeminfoValue(invalid, "MemTotal:", value));
    EXPECT_EQ(value, -999);
    EXPECT_FALSE(metric_blob::parseMeminfoValue(invalid, "x", value));
    EXPECT_EQ(value, -999);
}

TEST(ParseProcUptime, validInput)
{
    const std::string_view content = "266923.67 512184.95";
    const double eps =
        1e-4; // Empirical threshold for floating point number compare
    double uptime, idleProcessTime;
    EXPECT_EQ(metric_blob::parseProcUptime(content, uptime, idleProcessTime),
              true);
    EXPECT_LT(abs(uptime - 266923.67), eps);
    EXPECT_LT(abs(idleProcessTime - 512184.95), eps);
}

TEST(TrimStringRight, nonEmptyResult)
{
    EXPECT_EQ(
        metric_blob::trimStringRight("\n\nabc\n\t\r\x00\x01\x02\x03").size(),
        5); // "\n\nabc" is left
}

TEST(TrimStringRight, trimToEmpty)
{
    EXPECT_TRUE(metric_blob::trimStringRight("    ").empty());
    EXPECT_TRUE(metric_blob::trimStringRight("").empty());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}