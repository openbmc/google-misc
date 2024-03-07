#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
constexpr auto statusFile = "/run/dhcp_status";

// Function to read contents from a file
std::string fileRead(const fs::path& filename);

// Function to write contents to a file atomically
void fileWrite(const fs::path& filename, const std::string& data);
