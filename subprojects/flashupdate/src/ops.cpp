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

info::Status fetchInfo(const Args& args)
{
    std::filesystem::path path(args.config.eeprom.path);
    uint32_t size = std::filesystem::file_size(path);

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
                       args.config.eeprom.offset, mutate, sizeof(info::Status),
                       std::nullopt);

    std::vector<std::byte> fileIn(sizeof(info::Status));
    auto readFile = flasher::openFile(fileMod, OpenFlags(OpenAccess::ReadOnly));
    readFile->readAt(fileIn, 0);

    // Convert bytes to struct and update the state
    auto status = reinterpret_cast<info::Status*>(fileIn.data());
    return *status;
}

void writeInfo(const Args& args, const std::vector<std::byte>& buffer)
{
    flasher::NestedMutate mutate{};
    std::filesystem::path path(args.config.eeprom.path);
    uint32_t size = std::filesystem::file_size(path);

    auto devMod = ModArgs(fmt::format("fake,type=nor,erase={},{}", size,
                                      args.config.eeprom.path));
    auto fileMod = flasher::ModArgs("/tmp/temp-eeprom");

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
void info(const Args& args)
{
    auto info = fetchInfo(args);
    info::printStatus(args, info);
}

void updateState(const Args& args)
{
    auto findState = info::stringToState.find(args.state);
    if (findState == info::stringToState.end())
    {
        throw std::runtime_error(
            fmt::format("{} is not a supported state. Need to be one of\n{}",
                        args.state, info::listStates()));
    }

    auto info = fetchInfo(args);
    info::printStatus(args, info);

    info.state = static_cast<uint8_t>(findState->second);

    // Convert struct into bytes
    auto ptr = reinterpret_cast<std::byte*>(&info);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::Status));
    info::printStatus(args, info);

    writeInfo(args, buffer);
}

void updateStagedVersion(const Args& args)
{
    auto info = fetchInfo(args);
    info::printStatus(args, info);

    std::string image = args.file->arr.back();
    std::filesystem::path path(image);
    uint32_t size = std::filesystem::file_size(path);
    cr51::Cr51 helper(image, size, args.config.flash.validationKey);

    info.stage = info::Version(helper.imageVersion());

    // Convert struct into bytes
    auto ptr = reinterpret_cast<std::byte*>(&info);
    auto buffer = std::vector<std::byte>(ptr, ptr + sizeof(info::Status));
    info::printStatus(args, info);

    writeInfo(args, buffer);
}

void injectPersistent(const Args& args)
{
    std::string image = args.file->arr.back();
    std::filesystem::path path(image);
    uint32_t size = std::filesystem::file_size(path);
    cr51::Cr51 helper(image, size, args.config.flash.validationKey);
    auto regions = helper.persistentRegions();

    flash::Flash flashHelper(args.config, args.keepMux);
    auto flash = flashHelper.getFlash(true);
    if (!flash)
    {
        throw std::runtime_error("failed to find BIOS partitionS");
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
    if (!helper.verify(helper.prodImage()))
    {
        throw std::runtime_error(
            "invalid image after persistent regions injection");
    }
}

void hashDescriptor(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void read(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void write(const Args&)
{
    throw std::runtime_error("Not implemented");
}

} // namespace ops
} // namespace flashupdate
