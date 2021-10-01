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

#include <fmt/format.h>

#include <flashupdate/flash.hpp>
#include <flashupdate/flash/mock.hpp>

#include <fstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace flashupdate
{

TEST(FlashHelperTest, readMtdFileNoData)
{
    flash::FlashHelper flashHepler;

    std::string filename = "test_no_data.txt";
    std::ofstream testfile;

    testfile.open(filename.data(), std::ios::out);
    testfile << "";
    testfile.flush();
    EXPECT_THROW(
        try {
            flashHepler.readMtdFile(filename);
        } catch (const std::runtime_error& e) {
            EXPECT_STREQ("read: No data available", e.what());
            throw;
        },
        std::runtime_error);
    testfile.close();
}

TEST(FlashHelperTest, readMtdFileNoNewLine)
{
    flash::FlashHelper flashHepler;
    std::string filename = "test_no_newline.txt";
    std::ofstream testfile;
    std::vector<std::string_view> tests = {
        "test",
        "1123213s  asd",
        "  4234",
        "      ",
    };

    for (const auto& test : tests)
    {
        testfile.open(filename.data(), std::ios::out);
        testfile << test.data();
        testfile.flush();
        EXPECT_THROW(
            try {
                flashHepler.readMtdFile(filename);
            } catch (const std::runtime_error& e) {
                EXPECT_STREQ("not able to find newline in the mtd file",
                             e.what());
                throw;
            },
            std::runtime_error);
        testfile.close();
    }
}

TEST(FlashHelperTest, readMtdFilePass)
{
    flash::FlashHelper flashHepler;
    std::string filename = "test_pass.txt";
    std::ofstream testfile;
    std::vector<std::pair<std::string_view, std::string_view>> tests = {
        {"1234SS       \n      ", "1234SS       "},
        {"hello world\n", "hello world"},
        {"  xyz@@   \n      ", "  xyz@@   "},
        {"yes\n  dds  \n  ", "yes"},
        {"4123\n  dds ", "4123"},
        {"\n", ""},
    };

    for (const auto& test : tests)
    {
        testfile.open(filename.data(), std::ios::out);
        testfile << test.first.data();
        testfile.flush();
        EXPECT_EQ(flashHepler.readMtdFile(filename), test.second.data());
        testfile.close();
    }
}

class FlashTest : public ::testing::Test
{
  protected:
    FlashTest()
    {
        Config config;

        std::string dev("mtd,/dev/primary-flash");
        config.flash.primary = Config::Partition{
            "primary",
            primaryDev,
            std::nullopt,
        };

        config.flash.stagingIndex = 1;

        config.flash.secondary = {
            Config::Partition(),
            Config::Partition{
                "secondary",
                secondaryDev,
                1,
            },
        };
        flash = flash::Flash(config, false);
        flash.setFlashHelper(&flashHelper);
    }

    flash::Flash flash;
    std::string primaryDev = "mtd,/dev/primary-flash";
    std::string secondaryDev = "mtd,/dev/secondary-flash";
    flash::MockHelper flashHelper;
};

TEST_F(FlashTest, getFlashPrimaryMtd)
{
    std::string expectedName("primary");
    uint32_t expectedSize(324);

    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/primary-flash/name")))
        .WillOnce(Return(expectedName));
    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/primary-flash/size")))
        .WillOnce(Return(std::to_string(expectedSize)));

    EXPECT_EQ(flash.getFlash(/*primary=*/true),
              std::make_pair(primaryDev, expectedSize));
}

TEST_F(FlashTest, getFlashSecondaryMtd)
{
    std::string expectedName("secondary");
    uint32_t expectedSize(1234);

    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/secondary-flash/name")))
        .WillOnce(Return(expectedName));
    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/secondary-flash/size")))
        .WillOnce(Return(std::to_string(expectedSize)));

    EXPECT_EQ(flash.getFlash(/*primary=*/false),
              std::make_pair(secondaryDev, expectedSize));
}

TEST_F(FlashTest, getFlashMtdNameNotMatch)
{
    std::string expectedName("not-expected");

    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/secondary-flash/name")))
        .WillOnce(Return(expectedName));
    EXPECT_CALL(flashHelper,
                readMtdFile(std::string("/sys/class/mtd/primary-flash/name")))
        .WillOnce(Return(expectedName));

    EXPECT_EQ(flash.getFlash(/*primary=*/false), std::nullopt);
    EXPECT_EQ(flash.getFlash(/*primary=*/true), std::nullopt);
}

TEST_F(FlashTest, getFlashMtdCharNotFound)
{

    flash::FlashHelper flashHepler;
    std::string filename = "test.txt";
    std::string output = "123";
    std::string expectPath = fmt::format("fake,{}", filename);
    std::ofstream testfile;
    testfile.open(filename.data(), std::ios::out);
    testfile << output;
    testfile.flush();

    Config config;
    config.flash.primary = Config::Partition{
        "primary",
        expectPath,
        std::nullopt,
    };

    config.flash.stagingIndex = 1;

    config.flash.secondary = {
        Config::Partition(),
        Config::Partition{
            "secondary",
            "random/path",
            1,
        },
    };

    flash::Flash fakeFlash(config, false);

    EXPECT_EQ(fakeFlash.getFlash(/*primary=*/true),
              std::make_pair(expectPath, static_cast<uint32_t>(output.size())));

    // Expected to not find the path for secondary flash
    // Format is invalid
    EXPECT_EQ(fakeFlash.getFlash(/*primary=*/false), std::nullopt);
}

} // namespace flashupdate
