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

    // write should not create a file
    EXPECT_EQ(file_write(test_file, test_status), 1);

    // write create should pass
    EXPECT_EQ(file_write_create(test_file, test_status), 0);

    dhcp_status result_status;
    // read should get the same status
    EXPECT_EQ(file_read(test_file, result_status), 0);

    EXPECT_TRUE(test_status == result_status);

    // write should pass this time
    EXPECT_EQ(file_write(test_file, test_status_updated), 0);

    // read should get the same status
    EXPECT_EQ(file_read(test_file, result_status), 0);

    EXPECT_TRUE(test_status_updated == result_status);

    remove(test_file.c_str());
}
