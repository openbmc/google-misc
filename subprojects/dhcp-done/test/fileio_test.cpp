#include "../file-io.hpp"

#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>

#include <gtest/gtest.h>

TEST(TestFileIO, TestFileReadWrite)
{
    std::string testFile = "./tmp_test_file";

    std::string testStatus, testStatusUpdated;

    testStatus.push_back(2);
    testStatus.append("image downloading in progress");

    testStatusUpdated.push_back(0);
    testStatusUpdated.append("finished netboot");

    fileWrite(testFile, testStatus);

    EXPECT_TRUE(testStatus == fileRead(testFile));

    fileWrite(testFile, testStatusUpdated);

    EXPECT_TRUE(testStatusUpdated == fileRead(testFile));

    remove(testFile.c_str());
}
