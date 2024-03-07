#include "file-io.hpp"

#include <stdplus/print.hpp>

#include <cstring>
#include <iostream>

static void printUsage()
{
    std::cout << "Usage: update_dhcp_status <state> <message>" << std::endl;
    std::cout << "<state> is one of 'DONE', 'POWERCYCLE' or 'ONGOING'"
              << std::endl;
}

static int genStatus(char* state, char* msg, dhcp_status& result)
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
    if (argc != 3)
    {
        printUsage();
        return 1;
    }

    dhcp_status status;

    if (genStatus(argv[1], argv[2], status))
    {
        printUsage();
        return 1;
    }

    try
    {
        fileWrite(statusFile, status);
    }
    catch (...)
    {
        stdplus::println(stderr, "Failed to update status file");
        return 1;
    }

    return 0;
}
