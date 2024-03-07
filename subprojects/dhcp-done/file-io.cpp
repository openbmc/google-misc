#include "file-io.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <stdplus/fd/atomic.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/fmt.hpp>
#include <stdplus/fd/managed.hpp>
#include <stdplus/print.hpp>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

using ::stdplus::fd::ManagedFd;
using std::literals::string_view_literals::operator""sv;

// Function to read contents from a file (with locking)
void fileRead(const fs::path& filename, dhcp_status& content)
{
    // Open the file in read mode
    ManagedFd fd;
    fd = stdplus::fd::open(std::string(filename).c_str(), stdplus::fd::OpenAccess::ReadOnly);

    // Read the file contents
    std::ifstream file(filename);
    if (file.is_open())
    {
        if (!(file >> content.code))
        {
            stdplus::println(stderr,
                             "First element should be status code 0-255");
            throw std::runtime_error("Invalid status code");

        }
        // skip the new line
        std::getline(file, content.message);
        std::getline(file, content.message, '\0');
    }
    else
    {
        stdplus::println(stderr, "Failed to read file");
        throw std::runtime_error("File not open");
    }

}

// Function to write contents to a file atomically
void fileWrite(const fs::path& filename, const dhcp_status& status)
{

    stdplus::fd::AtomicWriter writer(filename, 0644);
    stdplus::fd::FormatBuffer out(writer);
    out.appends(std::to_string(status.code), "\n"sv ,status.message);
    out.flush();
    writer.commit();
}

