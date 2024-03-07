#include "file-io.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <stdplus/print.hpp>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

// Function to read contents from a file (with locking)
int file_read(const std::string& filename, dhcp_status& content)
{
    // Open the file in read mode
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        stdplus::println(stderr, "Could not open file for reading");
        return 1;
    }

    // Attempt to acquire an exclusive lock
    if (flock(fd, LOCK_EX) == -1)
    {
        stdplus::println(stderr, "Could not acquire lock on file");
        close(fd); // Ensure file descriptor is closed
        return 1;
    }

    // Read the file contents
    std::ifstream file(filename);
    if (file.is_open())
    {
        if (!(file >> content.code))
        {
            stdplus::println(stderr,
                             "First element should be status code 0-255");
            return 1;
        }
        // skip the new line
        std::getline(file, content.message);
        std::getline(file, content.message, '\0');
    }
    else
    {
        stdplus::println(stderr, "Failed to read file");
        return 1;
    }

    // Release the lock
    flock(fd, LOCK_UN);
    close(fd);

    return 0;
}

// Function to write contents to a file (with locking)
int file_write_(const std::string& filename, const dhcp_status& status,
                bool create)
{
    // Open the file in write mode (create file if it doesn't exist and create
    // flag is true)
    int fd = open(filename.c_str(), O_WRONLY | O_TRUNC | (create ? O_CREAT : 0),
                  0666);
    if (fd == -1)
    {
        stdplus::println(stderr, "Could not open file for writing");
        return 1;
    }

    // Attempt to acquire an exclusive lock
    if (flock(fd, LOCK_EX) == -1)
    {
        stdplus::println(stderr, "Could not acquire lock on file");
        close(fd);
        return 1;
    }

    // Write the content to the file
    std::ofstream file(filename);
    if (file.is_open())
    {
        file << std::to_string(status.code) << '\n' << status.message;
    }
    else
    {
        stdplus::println(stderr, "Failed to write to file");
        flock(fd, LOCK_UN); // Release lock on error
        close(fd);
        return 1;
    }

    // Release the lock
    flock(fd, LOCK_UN);
    close(fd);

    return 0;
}

int file_write_create(const std::string& filename, const dhcp_status& status)
{
    return file_write_(filename, status, true);
}

int file_write(const std::string& filename, const dhcp_status& status)
{
    return file_write_(filename, status, false);
}
