#include "file-io.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <stdplus/fd/atomic.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/fmt.hpp>
#include <stdplus/fd/managed.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/print.hpp>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

using ::stdplus::fd::ManagedFd;
using std::literals::string_view_literals::operator""sv;

// Function to read contents from a file (with locking)
std::string fileRead(const fs::path& filename)
{
    // Open the file in read mode
    ManagedFd fd = stdplus::fd::open(std::string(filename).c_str(),
                                     stdplus::fd::OpenAccess::ReadOnly);
    return stdplus::fd::readAll<std::string>(fd);
}

// Function to write contents to a file atomically
void fileWrite(const fs::path& filename, const std::string& data)
{
    stdplus::fd::AtomicWriter writer(filename, 0644);
    stdplus::fd::FormatBuffer out(writer);
    out.appends(data);
    out.flush();
    writer.commit();
}
