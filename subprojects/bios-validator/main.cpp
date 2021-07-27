/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cr51_image_descriptor.h"
#include "libcr51sign.h"
#include "libcr51sign_support.h"

#include "cli.hpp"
#include "log_utils.hpp"

#include <asm-generic/errno.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#ifndef ALLOW_PROD_TO_DEV_DOWNGRADE
#define ALLOW_PROD_TO_DEV_DOWNGRADE false
#endif

#ifndef IS_PRODUCTION_MODE
#define IS_PRODUCTION_MODE true
#endif

namespace
{

using ::fmt::format;
using ::google::bios_validator::CommandLine;
using ::google::bios_validator::kValidatorCMD;
using ::google::bios_validator::ValidatorArgs;
using ::google::bios_validator::internal::formatImageVersion;
using ::phosphor::logging::level;
using ::phosphor::logging::log;

constexpr int kSignatureRsa4096Pkcs15KeyLength = 512;
constexpr int kInvalidImageType = 4;

/* Global buffer to store BIOS file content. */
std::vector<uint8_t> biosFileBuf = {0};

bool prodToDevDowngradeAllowed()
{
    return ALLOW_PROD_TO_DEV_DOWNGRADE;
}

bool isProductionModeTrue()
{
    return IS_PRODUCTION_MODE;
}

int readFromBuf(const void*, uint32_t offset, uint32_t count, uint8_t* buf)
{
    if (count == 0)
        return LIBCR51SIGN_SUCCESS;
    if (!buf)
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    std::memcpy(buf, biosFileBuf.data() + offset, count);
    return LIBCR51SIGN_SUCCESS;
}

int validateDescriptor(struct libcr51sign_ctx* ctx)
{
    struct libcr51sign_intf intf;
    enum libcr51sign_validation_failure_reason ec;

    /* Common functions are from the libcr51sign support header. */
    intf.hash_init = &hash_init;
    intf.hash_update = &hash_update;
    intf.hash_final = &hash_final;
    intf.verify_signature = &verify_signature;
    intf.read_and_hash_update = nullptr;
    intf.read = &readFromBuf;
    intf.prod_to_dev_downgrade_allowed = &prodToDevDowngradeAllowed;
    intf.is_production_mode = &isProductionModeTrue;

    ec = libcr51sign_validate(ctx, &intf);
    if (ec != LIBCR51SIGN_SUCCESS)
    {
        std::string msg =
            format("Validate error: {}", libcr51sign_errorcode_to_string(ec));
        log<level::ERR>(msg.c_str());
        return ec;
    }
    return LIBCR51SIGN_SUCCESS;
}

void saveImageVersion(const struct libcr51sign_ctx& context,
                      const std::string& filename)
{
    std::ofstream version(filename.c_str(), std::ofstream::out);
    version << formatImageVersion(context.descriptor);
}

} // namespace

int validatorMain(const ValidatorArgs& args)
{
    /* Read the whole file into buffer.
     * Compared to implementing POSIX typed 'read' operations, reading SPI flash
     * contents into memory makes it easier to contruct input streams needed
     * later. The memory allocated will be freed afterwards. If the SPI size
     * increases in future platforms and makes BMC run-time memory at risk then
     * this can be revisited.
     */
    int fd = open(args.biosFilename.data(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return -1;

    biosFileBuf.reserve(args.biosFileSize);

    for (size_t readTotal = 0; readTotal < args.biosFileSize;)
    {
        ssize_t ret = read(fd, biosFileBuf.data(), args.biosFileSize);
        if (ret <= 0 && errno != EINTR && errno != EAGAIN &&
            errno != EWOULDBLOCK)
        {
            log<level::ERR>("Failed to read BIOS file to buffer!");
            close(fd);
            return EXIT_FAILURE;
        }
        readTotal += ret;
    }
    close(fd);

    struct hash_ctx shaContext;
    struct libcr51sign_ctx context;
    context.start_offset = 0,
    context.end_offset = static_cast<uint32_t>(args.biosFileSize),
    context.current_image_family = IMAGE_FAMILY_ALL,
    context.current_image_type = IMAGE_PROD,
    context.keyring_len = kSignatureRsa4096Pkcs15KeyLength,
    context.keyring = args.keyFilename.data(), context.priv = &shaContext;

    auto validateStatus = validateDescriptor(&context);
    if (validateStatus != LIBCR51SIGN_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    log<level::INFO>("BIOS CR51 Descriptor Validation Succeeds!");

    if (args.writeVersion)
    {
        saveImageVersion(context, args.versionFilename);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    CommandLine cli;
    int status = cli.parseArgs(argc, const_cast<const char**>(argv));
    if (status != EXIT_SUCCESS)
    {
        return status;
    }

    if (cli.gotSubcommand(kValidatorCMD))
    {
        ValidatorArgs args = cli.getArgs();
        return validatorMain(args);
    }
    log<level::ERR>("The subcommand is not supported!");
    return EXIT_FAILURE;
}
