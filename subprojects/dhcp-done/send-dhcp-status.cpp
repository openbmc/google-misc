#include "file-io.hpp"

#include <stdplus/print.hpp>

#include <cstring>
#include <iostream>

static void printUsage()
{
    stdplus::println(
        stderr,
        "Usage: send_dhcp_status <State> <Time> <Message> <Error Code> <Next State> <Retries>");
    stdplus::println(stderr, "Note: <Retries> is optional");
}

int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        printUsage();
        return 1;
    }

    try
    {
        std::string status;
        status.append("{'State':'");
        status.append(argv[1]);
        status.append("','Time':");
        status.append(argv[2]);
        status.append("','Message':");
        status.append(argv[3]);
        status.append("','Error Code':");
        status.append(argv[4]);
        status.append("','Next State':");
        status.append(argv[5]);
        if (argc > 6)
        {
            status.append("','retries':");
            status.append(argv[6]);
        }
        status.append("'}");
        fileWrite(statusFile, status);
    }
    catch (const std::exception& e)
    {
        stdplus::println(stderr, "Failed to update status file {}", e.what());
        return 1;
    }

    return 0;
}
