#pragma once
#include <string>

struct dhcp_status
{
    int code;
    std::string message;

    friend bool operator<=>(const dhcp_status&, const dhcp_status&) = default;
};

// Function to read contents from a file (with locking)
int file_read(const std::string& filename, dhcp_status& status);

// Function to create and write contents to a file (with locking)
int file_write_create(const std::string& filename, const dhcp_status& status);

// Function to write contents to a file (with locking)
int file_write(const std::string& filename, const dhcp_status& status);
