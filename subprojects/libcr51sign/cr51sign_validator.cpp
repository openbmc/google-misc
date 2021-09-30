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

#include "cr51sign_validator.h"

#include "libcr51sign.h"
#include "libcr51sign_support.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <optional>
#include <vector>

#ifndef ALLOW_PROD_TO_DEV_DOWNGRADE
#define ALLOW_PROD_TO_DEV_DOWNGRADE false
#endif

#ifndef IS_PRODUCTION_MODE
#define IS_PRODUCTION_MODE true
#endif

namespace google
{
namespace cr51
{

/* Global buffer to store firmware content. */
std::vector<uint8_t> firmwareFileBuf = {0};

std::vector<uint8_t>& getBuffer()
{
    return firmwareFileBuf;
}

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
    std::memcpy(buf, firmwareFileBuf.data() + offset, count);
    return LIBCR51SIGN_SUCCESS;
}

bool readImage(const char* fileName, size_t size)
{
    /* Read the whole file into buffer.
     * Compared to implementing POSIX typed 'read' operations, reading SPI flash
     * contents into memory makes it easier to construct input streams needed
     * later. The memory allocated will be freed afterwards. If the SPI size
     * increases in future platforms and makes BMC run-time memory at risk then
     * this can be revisited.
     */
    int fd = open(fileName, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return -1;

    firmwareFileBuf.clear();
    firmwareFileBuf.reserve(size);

    for (size_t readTotal = 0; readTotal < size;)
    {
        ssize_t ret = read(fd, firmwareFileBuf.data(), size);
        if (ret <= 0 && errno != EINTR && errno != EAGAIN &&
            errno != EWOULDBLOCK)
        {
            fmt::print(stderr, "Failed to read BIOS file to buffer: ec{}",
                       EXIT_FAILURE);
            close(fd);
            return false;
        }
        readTotal += ret;
    }
    close(fd);

    return true;
}

std::optional<std::vector<uint8_t>> hashDescriptor(struct libcr51sign_ctx* ctx)
{
    // Create the HASH of the CR51 descriptor on the firmware
    size_t size = 0;
    hash_type hashType = static_cast<hash_type>(ctx->descriptor.hash_type);

    switch (hashType)
    {
        case HASH_SHA2_224:
        case HASH_SHA3_224:
            size = SHA224_DIGEST_LENGTH;
            break;
        case HASH_SHA2_256:
        case HASH_SHA3_256:
            size = SHA256_DIGEST_LENGTH;
            break;
        case HASH_SHA2_384:
        case HASH_SHA3_384:
            size = SHA384_DIGEST_LENGTH;
            break;
        case HASH_SHA2_512:
        case HASH_SHA3_512:
            size = SHA512_DIGEST_LENGTH;
            break;
        case HASH_NONE:
        default:
            fmt::print(stderr, "CR51 Hash type is not supported: type {}",
                       hashType);
            return std::nullopt;
            break;
    }

    std::vector<uint8_t> hash(size);
    int ec;
    ec = hash_init(ctx, hashType);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash init error: ec{}", ec);
        return std::nullopt;
    }

    hash_update(ctx, firmwareFileBuf.data() + ctx->descriptor.descriptor_offset,
                ctx->descriptor.descriptor_area_size);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash update error: ec{}", ec);
        return std::nullopt;
    }

    hash_final(ctx, hash.data());
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash final error: ec{}", ec);
        return std::nullopt;
    }

    return hash;
}

std::optional<struct libcr51sign_validated_regions>
    validateDescriptor(struct libcr51sign_ctx* ctx)
{
    struct libcr51sign_intf intf;
    struct libcr51sign_validated_regions imageRegions;
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

    ec = libcr51sign_validate(ctx, &intf, &imageRegions);
    if (ec != LIBCR51SIGN_SUCCESS)
    {
        fmt::print(stderr, "CR51 Validate error: {}",
                   libcr51sign_errorcode_to_string(ec));
        return std::nullopt;
    }

    return imageRegions;
}

} // namespace cr51
} // namespace google
