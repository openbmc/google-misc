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
    std::string test_file = "./tmp_test_file";

    dhcp_status test_status{
        .code = 2,
        .message = "image downloading in progress",
    };

    dhcp_status test_status_updated{
        .code = 0,
        .message = "finished netboot",
    };

    fileWrite(test_file, test_status);

    dhcp_status result_status;

    // read should get the same status
    fileRead(test_file, result_status);

    EXPECT_TRUE(test_status == result_status);

    fileWrite(test_file, test_status_updated);

    // read should get the same status
    fileRead(test_file, result_status);

    EXPECT_TRUE(test_status_updated == result_status);

    remove(test_file.c_str());
}
