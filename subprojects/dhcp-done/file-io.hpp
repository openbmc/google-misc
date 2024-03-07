#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
constexpr auto statusFile = "/run/dhcp_status";

struct dhcp_status
{
    int code;
    std::string message;

    friend bool operator<=>(const dhcp_status&, const dhcp_status&) = default;
};

// Function to read contents from a file
void fileRead(const fs::path& filename, dhcp_status& status);

// Function to write contents to a file atomically
void fileWrite(const fs::path& filename, const dhcp_status& status);
