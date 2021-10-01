// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fmt/format.h>
#include <unistd.h>

#include <flasher/file.hpp>
#include <flasher/mutate.hpp>
#include <flasher/ops.hpp>
#include <flashupdate/args.hpp>
#include <flashupdate/flash.hpp>
#include <flashupdate/logging.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace flashupdate
{
namespace flash
{

using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlag;
using stdplus::fd::OpenFlags;

// Flash helper functions

/** @brief Read the contect in the mtd file
 *
 * @return list of Firmware states as string
 */
std::string readMtdFile(std::string_view filename)
{
    auto argReadFile = flasher::ModArgs(filename.data());
    auto readFile = openFile(argReadFile, OpenFlags(OpenAccess::ReadOnly));

    std::filesystem::path file(filename.data());
    auto size = std::filesystem::file_size(file);

    std::vector<std::byte> fileIn(size);
    readFile->readAt(fileIn, 0);

    // Find new line and remove add data after it.
    auto it =
        std::find(fileIn.begin(), fileIn.end(), static_cast<std::byte>(0x0a));
    if (it == fileIn.end())
    {
        throw std::runtime_error("not able to find newline in the mtd file");
    }

    size = it - fileIn.begin();
    std::string output(' ', size);

    std::memcpy(output.data(), fileIn.data(), size);
    output.resize(size);
    return output;
}

// BIOS helper class definitions
Flash::Flash(Config config, bool keepMux) : config(config), keepMux(keepMux)
{
    try
    {
        cleanup();
    }
    catch (const std::exception& e)
    {
        log(LogLevel::Warning,
            "WARNING: cleanup before searching for the flash location to make "
            "sure it does not fail: {}\n",
            e.what());
    }
}

Flash::~Flash()
{
    if (!keepMux)
    {
        cleanup();
    }
}

std::optional<std::pair<std::string, uint32_t>> Flash::getFlash(bool primary)
{
    auto partition = primary ? config.flash.primary
                             : config.flash.secondary[config.flash.stagingIndex];

#ifndef DEV_WORKFLOW
    std::string gpio =
        fmt::format("/sys/class/gpio/gpio{}/", *config.flash.primary.muxSelect);

    log(LogLevel::Info, "INFO: Select the MUX with {}\n", gpio);

    if (access(std::string(gpio).c_str(), F_OK) == -1)
    {
        auto argFile = flasher::ModArgs("/sys/class/gpio/export");
        auto file = openFile(argFile, OpenFlags(OpenAccess::WriteOnly)
                                          .set(OpenFlag::Create)
                                          .set(OpenFlag::Trunc));
        std::string data = std::to_string(*config.flash.primary.muxSelect);
        std::vector<std::byte> file_out(data.size());
        std::memcpy(file_out.data(), data.data(), data.size());

        file->writeAtExact(file_out, 0);
    }

    if (partition.muxSelect)
    {
        log(LogLevel::Info,
            "INFO: Select the MUX with gpio{} to enable the BIOS flash\n",
            *partition.muxSelect);

        auto argFile = flasher::ModArgs(fmt::format(
            "/sys/class/gpio/gpio{}/direction", *partition.muxSelect));
        auto file = openFile(argFile, OpenFlags(OpenAccess::WriteOnly)
                                          .set(OpenFlag::Create)
                                          .set(OpenFlag::Trunc));
        std::string data = "high";
        std::vector<std::byte> file_out(data.size());
        std::memcpy(file_out.data(), data.data(), data.size());
        file->writeAtExact(file_out, 0);
    }

    auto argFile = flasher::ModArgs(fmt::format("{}/bind", config.flash.driver));
    auto file = openFile(argFile, OpenFlags(OpenAccess::WriteOnly)
                                      .set(OpenFlag::Create)
                                      .set(OpenFlag::Trunc));
    auto file_out = std::vector<std::byte>(config.flash.deviceId.size());
    memcpy(file_out.data(), config.flash.deviceId.data(),
           config.flash.deviceId.size());
    file->writeAtExact(file_out, 0);
    log(LogLevel::Info, "INFO: binded {} to {}\n", config.flash.deviceId,
        config.flash.driver);
#endif

    std::string_view location = partition.location;

    std::string_view check(location.substr(0, 3));
    // if (location.starts_with("mtd"))
    if (check != "mtd")
    {
        // non-dev path is expected to be in the format of
        //   fake,type=nor,erase=4096,fake.img
        // Last element is the image
        auto index = location.find_last_of(',');
        if (index == std::string::npos)
        {
            return std::nullopt;
        }

        std::filesystem::path path(location.substr(index + 1));
        uint32_t size = std::filesystem::file_size(path);
        return std::make_pair(partition.location, size);
    }

    auto index = location.find_last_of('/');
    if (index == std::string::npos)
    {
        return std::nullopt;
    }

    // Validate the mtd name
    auto mtd = location.substr(index + 1);

    auto name = readMtdFile(fmt::format("/sys/class/mtd/{}/name", mtd));
    if (name != std::string_view(partition.name))
    {
        return std::nullopt;
    }

    uint32_t size =
        std::stoi(readMtdFile(fmt::format("/sys/class/mtd/{}/size", mtd)));

    log(LogLevel::Info, "INFO: using {} as the BIOS flash with size of {}\n",
        name, size);

    return std::make_pair(partition.location, size);
}

void Flash::cleanup()
{
#ifndef DEV_WORKFLOW
    log(LogLevel::Info, "INFO: Cleanup the BIOS MUX\n");
    log(LogLevel::Info, "INFO: unbind {} to {}\n", config.flash.deviceId,
        config.flash.driver);

    auto argFile =
        flasher::ModArgs(fmt::format("{}/unbind", config.flash.driver));
    auto file = openFile(argFile, OpenFlags(OpenAccess::WriteOnly)
                                      .set(OpenFlag::Create)
                                      .set(OpenFlag::Trunc));
    std::vector<std::byte> file_out =
        std::vector<std::byte>(config.flash.deviceId.size());
    std::memcpy(file_out.data(), config.flash.deviceId.data(),
                config.flash.deviceId.size());
    file->writeAtExact(file_out, 0);

    // # Switch mux to host
    log(LogLevel::Info, "INFO: set gpio{} to low\n",
        *config.flash.primary.muxSelect);
    argFile = flasher::ModArgs(fmt::format("/sys/class/gpio/gpio{}/direction",
                                           *config.flash.primary.muxSelect));
    file = openFile(argFile, OpenFlags(OpenAccess::WriteOnly)
                                 .set(OpenFlag::Create)
                                 .set(OpenFlag::Trunc));
    std::string data = "low";
    file_out = std::vector<std::byte>(data.size());
    memcpy(file_out.data(), data.data(), data.size());
    file->writeAtExact(file_out, 0);
#endif
}

} // namespace flash
} // namespace flashupdate