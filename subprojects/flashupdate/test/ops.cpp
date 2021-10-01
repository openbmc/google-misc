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

#include <flashupdate/cr51/mock.hpp>
#include <flashupdate/flash/mock.hpp>
#include <flashupdate/info.hpp>
#include <flashupdate/ops.hpp>

#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace flashupdate
{

class OperationTest : public ::testing::Test
{
  protected:
    OperationTest()
    {
        resetInfo();

        std::ofstream testfile;
        testfile.open(testBin.data(), std::ios::out);
        testfile << inputData.data();
        testfile.flush();
        testfile.close();

        testDev =
            fmt::format("fake,type=nor,erase={},{}", inputData.size(), testBin);
    }

    void resetInfo()
    {
        updateInfo.active = info::Version(activeVersion);
        updateInfo.stage = info::Version(stageVersion);
        updateInfo.stagingIndex = 3;
        updateInfo.state = 2;
    }

    void createFakeEeprom(std::string_view filename)
    {
        auto ptr = reinterpret_cast<std::byte*>(&updateInfo);
        auto buffer =
            std::vector<std::byte>(ptr, ptr + sizeof(info::UpdateInfo));
        std::ofstream testEeprom;

        testEeprom.open(filename.data(), std::ios::out);
        testEeprom << buffer.data();
        testEeprom.flush();
        testEeprom.close();
    }

    info::UpdateInfo updateInfo;
    std::string_view activeVersion = "10.11.12.13";
    std::string_view stageVersion = "4.3.2.1";
    std::string_view testBin = "test.bin";
    std::string_view inputData = "hello world";
    std::string testDev;
};

TEST_F(OperationTest, InfoPass)
{
    std::string filename = "info_test_eeprom";
    std::string expectedOutput = "";
    resetInfo();
    createFakeEeprom(filename);

    Args args;
    args.config.eeprom.path = filename;

    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkActiveVersion = true;
    expectedOutput += fmt::format("Active Version: {}\n", activeVersion.data());
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkStageVersion = true;
    expectedOutput += fmt::format("Stage Version: {}\n", stageVersion.data());
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.checkStageState = true;
    expectedOutput += "Status Staged State: CORRUPTED\n";
    EXPECT_EQ(ops::info(args), expectedOutput);

    args.otherInfo = true;
    expectedOutput += "Staging Index: 0\n";
    expectedOutput += "CR51 Descriptor Hash: 00000000000000000000000\n";
    EXPECT_EQ(ops::info(args), expectedOutput);

    expectedOutput =
        fmt::format("{}\n{}\n{}\n{}\n{}\n", activeVersion, stageVersion,
                    "CORRUPTED", "0", "00000000000000000000000");
    EXPECT_EQ(ops::info(args), expectedOutput);
}

TEST_F(OperationTest, UpdateStateInvalidState)
{
    Args args;

    args.state = "FAKE_STATE";
    EXPECT_THROW(
        try { ops::updateState(args); } catch (const std::runtime_error& e) {
            EXPECT_STREQ(
                e.what(),
                fmt::format(
                    "{} is not a supported state. Need to be one of\n{}",
                    args.state)
                    .c_str());
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, UpdateStatePass)
{
    std::string filename = "update_state_eeprom";
    resetInfo();
    createFakeEeprom(filename);

    Args args;
    args.config.eeprom.path = filename;
    args.checkStageState = true;
    args.state = "STAGED";

    EXPECT_EQ(ops::updateState(args),
              fmt::format("Status Staged State: {}\n", "STAGED"));
}

TEST_F(OperationTest, UpdateStagedVersion)
{
    std::string filename = "update_staged_version_eeprom";
    resetInfo();
    createFakeEeprom(filename);

    Args args;
    args.config.eeprom.path = filename;
    args.checkStageVersion = true;

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    // Return newVersion to to the EEPROM
    // The current staged version is `stagedVersion` and we want to write to
    // something else.
    std::string_view newVersion = "9.9.10.11";
    EXPECT_CALL(cr51MockHelper, imageVersion())
        .WillOnce(Return(newVersion.data()));
    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));

    EXPECT_EQ(ops::updateStagedVersion(args),
              fmt::format("Stage Version: {}\n", newVersion));
}

TEST_F(OperationTest, InjectPersistentInvalidImage)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(false));

    EXPECT_THROW(
        try {
            ops::injectPersistent(args);
        } catch (const std::runtime_error& e) {
            EXPECT_STREQ(
                e.what(),
                fmt::format("failed to validate the CR51 descriptor for {}",
                            testBin)
                    .c_str());
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, InjectPersistentNoFlashPartition)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(flashMockHelper, getFlash(true)).WillOnce(Return(std::nullopt));

    EXPECT_THROW(
        try {
            ops::injectPersistent(args);
        } catch (const std::runtime_error& e) {
            EXPECT_STREQ(e.what(), "failed to find Flash partitions");
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, InjectPersistentNoPersistentRegionPass)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, persistentRegions())
        .WillOnce(Return(std::vector<struct image_region>()));
    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(cr51MockHelper, verify(_)).WillOnce(Return(true));

    EXPECT_CALL(flashMockHelper, getFlash(true))
        .WillOnce(Return(std::make_pair(testDev, inputData.size())));

    ops::injectPersistent(args);
}

TEST_F(OperationTest, InjectPersistentNoPersistentRegionInvalidAfter)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, persistentRegions())
        .WillOnce(Return(std::vector<struct image_region>()));
    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(cr51MockHelper, verify(_)).WillOnce(Return(false));

    EXPECT_CALL(flashMockHelper, getFlash(true))
        .WillOnce(Return(std::make_pair(testDev, inputData.size())));

    EXPECT_THROW(
        try {
            ops::injectPersistent(args);
        } catch (const std::runtime_error& e) {
            EXPECT_STREQ(e.what(),
                         "invalid image after persistent regions injection");
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, InjectPersistentPass)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, persistentRegions())
        .WillOnce(Return(std::vector<struct image_region>{{}, {}}));
    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(cr51MockHelper, verify(_)).WillOnce(Return(true));

    EXPECT_CALL(flashMockHelper, getFlash(true))
        .WillOnce(Return(std::make_pair(testDev, inputData.size())));

    ops::injectPersistent(args);
}

TEST_F(OperationTest, HashDescriptorInvalidImage)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(false));

    EXPECT_THROW(
        try { ops::hashDescriptor(args); } catch (const std::runtime_error& e) {
            EXPECT_STREQ(
                e.what(),
                fmt::format("failed to validate the CR51 descriptor for {}",
                            testBin)
                    .c_str());
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, HashDescriptorPass)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    std::string expectedHashStr =
        "000100000000000000000c000000000000000000000000000000000000003000";
    std::vector<uint8_t> expectedHash(32);
    expectedHash[1] = 1;
    expectedHash[10] = 12;
    expectedHash[30] = 48;

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(cr51MockHelper, descriptorHash())
        .WillOnce(Return(expectedHash));

    EXPECT_EQ(ops::hashDescriptor(args), expectedHashStr);
}

TEST_F(OperationTest, ReadInvalidFlash)
{
    Args args;
    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(flashMockHelper, getFlash(_)).WillOnce(Return(std::nullopt));

    EXPECT_THROW(
        try { ops::read(args); } catch (const std::runtime_error& e) {
            EXPECT_STREQ(e.what(), "failed to find Flash partitions");
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, ReadInvalidImage)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(false));
    EXPECT_CALL(flashMockHelper, getFlash(_))
        .WillOnce(Return(std::make_pair(testDev, inputData.size())));

    EXPECT_THROW(
        try { ops::read(args); } catch (const std::runtime_error& e) {
            EXPECT_STREQ(
                e.what(),
                fmt::format("failed to validate the CR51 descriptor for {}",
                            testBin)
                    .c_str());
            throw;
        },
        std::runtime_error);
}

TEST_F(OperationTest, ReadPass)
{
    Args args;
    args.file = flasher::ModArgs(testBin);

    cr51::Mock cr51MockHelper;
    args.setCr51Helper(&cr51MockHelper);

    flash::Mock flashMockHelper;
    args.setFlashHelper(&flashMockHelper);

    EXPECT_CALL(cr51MockHelper, validateImage(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(flashMockHelper, getFlash(_))
        .WillOnce(Return(std::make_pair(testDev, inputData.size())));

    ops::read(args);
}

} // namespace flashupdate
