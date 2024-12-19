#include "file-io.hpp"
#include <stdplus/print.hpp>
#include <cstring>
#include <iostream>
#include <syslog.h>
static void printUsage()
{
    stdplus::println(stderr, "Usage: update_dhcp_status <state> <message>");
    stdplus::println(stderr,
                     "<state> is one of 'DONE', 'POWERCYCLE' or 'ONGOING'");
    stdplus::println(
        stderr,
        "New Usage: update_dhcp_status <State> <Time> <Message> <Code> <Retries>");
    stdplus::println(
        stderr,
        "Note: <Retries> is optional, hardcoded to 2 logging unlike legacy state");
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

    return -1;
}

static int ThreeArgsProc(char* argv[])
{
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

int StructuredArgsProc(int argc, char* argv[])
{
    try
    {
        std::string status;
        status.append("{\"State\":\"");
        status.append(argv[1]);
        status.append("\",\"Time\":\"");
        status.append(argv[2]);
        status.append("\",\"Message\":\"");
        status.append(argv[3]);
        status.append("\",\"Code\":\"");
        status.append(argv[4]);
        if (argc > 5)
        {
            status.append("\",\"retries\":\"");
            status.append(argv[5]);
        }
        status.append("\"}");
        openlog("gbmc_netboot", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "%s", status.c_str());
	closelog();
	status.insert(0, 1, 2);
        fileWrite(statusFile, status);
    }
    catch (const std::exception& e)
    {
        stdplus::println(stderr, "Failed to update status file {}", e.what());
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3 && argc < 5)
    {
        printUsage();
        return 1;
    }

    if (argc == 3)
    {
        return ThreeArgsProc(argv);
    }
    else
    {
        return StructuredArgsProc(argc, argv);
    }
}
