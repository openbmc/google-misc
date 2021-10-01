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

#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/mod.hpp>
#include <flasher/mutate.hpp>
#include <flasher/ops.hpp>
#include <flashupdate/args.hpp>
#include <flashupdate/cr51.hpp>
#include <flashupdate/flash.hpp>
#include <flashupdate/info.hpp>
#include <flashupdate/logging.hpp>
#include <stdplus/exception.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace flashupdate
{
namespace ops
{

using flasher::ModArgs;
using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlag;
using stdplus::fd::OpenFlags;

/** @brief Read the information saved in the eeprom
 *
 * @param[in] args   Arguments containing all configuration infoamtion
 *
 * @return UpdateInfo that is saved in the eeprom
 */
static info::UpdateInfo fetchInfo(const Args& args)
{
    std::filesystem::path path(args.config.eeprom.path);
    size_t size = std::filesystem::file_size(path);

    auto devMod = ModArgs(fmt::format("fake,type=nor,erase={},{}", size,
                                      args.config.eeprom.path));
    auto fileMod = ModArgs("/tmp/temp-eeprom");
    flasher::NestedMutate mutate{};

    // Copy status from Motherboard EEPROM
    auto dev = flasher::openDevice(devMod);
    auto file = flasher::openFile(fileMod, OpenFlags(OpenAccess::WriteOnly)
                                               .set(OpenFlag::Create)
                                               .set(OpenFlag::Trunc));
    flasher::ops::read(*dev, args.config.eeprom.offset, *file,
                       args.config.eeprom.offset, mutate,
                       sizeof(info::UpdateInfo), std::nullopt);

    std::vector<std::byte> fileIn(sizeof(info::UpdateInfo));
    auto readFile = flasher::openFile(fileMod, OpenFlags(OpenAccess::ReadOnly));
    readFile->readAt(fileIn, 0);

    // Convert bytes to struct and update the state
    auto status = reinterpret_cast<info::UpdateInfo*>(fileIn.data());
    return *status;
}

/** @brief Write the UpdateInfo to the EEPROM
 *
 * @param[in] args   Arguments containing all configuration infoamtion
 * @param[in] buffer Data to write to the EEPROM
 *
 * @return UpdateInfo that is saved in the eeprom
 */
static void writeInfo(const Args& args, const std::vector<std::byte>& buffer)
{
    std::filesystem::path path(args.config.eeprom.path);
    size_t size = std::filesystem::file_size(path);
    auto devMod = ModArgs(fmt::format("fake,type=nor,erase={},{}", size,
                                      args.config.eeprom.path));
    auto fileMod = flasher::ModArgs("/tmp/temp-eeprom");

    flasher::NestedMutate mutate{};
    auto dev = flasher::openDevice(devMod);
    auto file = flasher::openFile(fileMod, OpenFlags(OpenAccess::ReadWrite)
                                               .set(OpenFlag::Create)
                                               .set(OpenFlag::Trunc));
    file->writeAtExact(buffer, 0);
    flasher::ops::automatic(
        *dev, args.config.eeprom.offset, *file, args.config.eeprom.offset,
        mutate, std::numeric_limits<size_t>::max(), std::nullopt, false);
}

// Operation Definitions
std::string info(const Args& args)
{
    return info::printUpdateInfo(args, fetchInfo(args));
}

std::string updateState(const Args& args)
{
    auto findState = info::stringToState.find(args.state);
    if (findState == info::stringToState.end())
    {
        throw std::runtime_error(fmt::format(
            "{} is not a supported state. Need to be one of\n{}", args.state));
    }

    auto info = fetchInfo(args);
    info::printUpdateInfo(args, info);

    info.state = static_cast<uint8_t>(findState->second);

    // Convert struct into bytes
    auto ptr = reinterpret_cast<std::byte*>(&info);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::UpdateInfo));

    writeInfo(args, buffer);
    return info::printUpdateInfo(args, info);
}

std::string updateStagedVersion(const Args& args)
{
    auto info = fetchInfo(args);
    info::printUpdateInfo(args, info);

    std::string image = args.file->arr.back();
    std::filesystem::path path(image);
    uint32_t size = std::filesystem::file_size(path);

    // Validate the CR51 descriptor for the image
    // Parse the image to fetch the image version and other informations.
    auto& helper = args.cr51Helper;
    if (!helper->validateImage(image, size, args.config.flash.validationKey))
    {
        throw std::runtime_error(fmt::format(
            "failed to validate the CR51 descriptor for {}", image));
    }

    info.stage = info::Version(helper->imageVersion());

    // Convert struct into bytes
    auto ptr = reinterpret_cast<std::byte*>(&info);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::UpdateInfo));

    writeInfo(args, buffer);
    return info::printUpdateInfo(args, info);
}

void injectPersistent(const Args& args)
{
    std::string image = args.file->arr.back();
    std::filesystem::path path(image);
    uint32_t size = std::filesystem::file_size(path);
    auto& helper = args.cr51Helper;
    if (!helper->validateImage(image, size, args.config.flash.validationKey))
    {
        throw std::runtime_error(fmt::format(
            "failed to validate the CR51 descriptor for {}", image));
    }

    auto regions = helper->persistentRegions();
    auto& flashHelper = args.flashHelper;

    flashHelper->setup(args.config, args.keepMux);
    auto flash = flashHelper->getFlash(true);
    if (!flash)
    {
        throw std::runtime_error("failed to find Flash partitions");
    }

    log(LogLevel::Notice, "NOTICE: Inject Persistent from {} to {}\n",
        flash->first, image);

    auto dev = flasher::openDevice(flasher::ModArgs(flash->first));

    auto fileMod = *args.file;
    auto file = flasher::openFile(fileMod, OpenFlags(OpenAccess::ReadWrite));

    flasher::NestedMutate mutate{};

    for (const auto& region : regions)
    {
        log(LogLevel::Notice,
            "NOTICE: Region: {}, Inject offset: {}, Length: {}\n",
            region.region_name, region.region_offset, region.region_size);
        auto dev = flasher::openDevice(flasher::ModArgs(flash->first));
        flasher::ops::read(*dev, region.region_offset, *file,
                           region.region_offset, mutate, region.region_size,
                           std::nullopt);
    }

    // Validate the image again after injecting the persistent regions
    if (!helper->verify(helper->prodImage()))
    {
        throw std::runtime_error(
            "invalid image after persistent regions injection");
    }
}

std::string hashDescriptor(const Args& args)
{
    std::string image = args.file->arr.back();
    std::filesystem::path path(image);
    size_t size = std::filesystem::file_size(path);
    log(LogLevel::Info,
        "INFO: CR51 Descriptor HASH for BIOS image: {}, size: {}\n", image,
        size);
    auto& helper = args.cr51Helper;
    if (!helper->validateImage(image, size, args.config.flash.validationKey))
    {
        throw std::runtime_error(fmt::format(
            "failed to validate the CR51 descriptor for {}", image));
    }
    auto output = info::hashToString(helper->descriptorHash());
    fmt::print(stdout, output);
    return output;
}

void read(const Args& args)
{
    auto& flashHelper = args.flashHelper;
    flashHelper->setup(args.config, args.keepMux);

    auto flash = flashHelper->getFlash(args.primary);
    if (!flash)
    {
        throw std::runtime_error("failed to find Flash partitions");
    }

    std::string image = args.file->arr.back();
    flasher::NestedMutate mutate{};

    auto devMod = ModArgs(fmt::format("{}", flash->first));
    auto fileMod = *args.file;

    auto dev = flasher::openDevice(devMod);
    auto file = flasher::openFile(fileMod, OpenFlags(OpenAccess::WriteOnly)
                                               .set(OpenFlag::Create)
                                               .set(OpenFlag::Trunc));
    flasher::ops::read(*dev, 0, *file, 0, mutate,
                       std::numeric_limits<size_t>::max(), std::nullopt);

    // Validate the image read from the flash
    std::filesystem::path path(image);
    size_t size = std::filesystem::file_size(path);
    auto& helper = args.cr51Helper;
    if (!helper->validateImage(image, size, args.config.flash.validationKey))
    {
        throw std::runtime_error(fmt::format(
            "failed to validate the CR51 descriptor for {}", image));
    }
}

void write(const Args&)
{
    throw std::runtime_error("Not implemented");
}

} // namespace ops
} // namespace flashupdate
