#include "../config.h"

#include "file-io.hpp"

#include <stdplus/print.hpp>

#include <cstring>
#include <iostream>

void print_usage()
{
    std::cout << "Usage: update_dhcp_status <state> <message>" << std::endl;
    std::cout << "<state> is one of 'DONE', 'POWERCYCLE' or 'ONGOING'"
              << std::endl;
}

int gen_status(char* state, char* msg, dhcp_status& result)
{
    if (std::strcmp(state, "DONE") == 0)
    {
        result.code = 0;
    }
    else if (std::strcmp(state, "POWERCYCLE") == 0)
    {
        result.code = 1;
    }
    else if (std::strcmp(state, "ONGOING") == 0)
    {
        result.code = 2;
    }
    else
    {
        return 1;
    }

    result.message = msg;
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3 && argc != 4)
    {
        print_usage();
        return 1;
    }

    dhcp_status status;

    if (gen_status(argv[1], argv[2], status))
    {
        print_usage();
        return 1;
    }

    // internal cmd for file creation
    if (argc == 4)
    {
        if (std::strcmp(argv[3], "--create") != 0)
        {
            print_usage();
            return 1;
        }

        if (file_write_create(DHCP_STATUS_FILE_PATH, status))
        {
            stdplus::println(stderr, "Failed to create status file");
            return 1;
        }

        return 0;
    }

    if (file_write(DHCP_STATUS_FILE_PATH, status))
    {
        stdplus::println(stderr, "Failed to update status file");
        return 1;
    }

    return 0;
}
