#include "file-io.hpp"

#include <stdplus/print.hpp>

#include <cstring>
#include <iostream>

static void printUsage()
{
    stdplus::println(stderr, "Usage: update_dhcp_status <state> <message>");
    stdplus::println(
        stderr,
        "<state> is one of 'DONE', 'POWERCYCLE', 'REBOOT' or 'ONGOING'");
}

static int genStatusCode(char* state)
{
    if (std::strcmp(state, "DONE") == 0)
    {
        return 0;
    }
    else if (std::strcmp(state, "POWERCYCLE") == 0)
    {
        return 1;
    }
    else if (std::strcmp(state, "ONGOING") == 0)
    {
        return 2;
    }
    else if (std::strcmp(state, "REBOOT") == 0)
    {
        return 3;
    }

    return -1;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printUsage();
        return 1;
    }

    int statusCode = genStatusCode(argv[1]);

    if (statusCode == -1)
    {
        printUsage();
        return 1;
    }

    try
    {
        std::string status;
        status.push_back(statusCode);
        status.append(argv[2]);
        fileWrite(statusFile, status);
    }
    catch (const std::exception& e)
    {
        stdplus::println(stderr, "Failed to update status file {}", e.what());
        return 1;
    }

    return 0;
}
