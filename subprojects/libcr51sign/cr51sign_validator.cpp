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
#include <span>

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

std::span<const uint8_t> Cr51SignValidatorIpml::getBuffer()
{
    return firmwareFileBuf;
}

bool Cr51SignValidatorIpml::readImage(const char* fileName, size_t size)
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

std::span<const uint8_t>
    Cr51SignValidatorIpml::hashDescriptor(struct libcr51sign_ctx* ctx)
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
            return {};
            break;
    }

    hash.clear();
    hash.reserve(size);
    int ec;
    ec = hash_init(ctx, hashType);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash init error: ec{}", ec);
        return {};
    }

    hash_update(ctx, firmwareFileBuf.data() + ctx->descriptor.descriptor_offset,
                ctx->descriptor.descriptor_area_size);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash update error: ec{}", ec);
        return {};
    }

    hash_final(ctx, hash.data());
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash final error: ec{}", ec);
        return {};
    }

    return hash;
}

/** @struct Lambda
 *  @brief Convert lambda with capture variable to function pointer
 *
 * This is created based solely on https://stackoverflow.com/q/7852101
 */
struct Lambda
{
    template <typename Tret, typename T>
    static Tret lambda_ptr_exec(const void* data, uint32_t offset,
                                uint32_t count, uint8_t* buf)
    {
        return (Tret)(*(T*)fn<T>())(data, offset, count, buf);
    }

    template <typename Tret = int,
              typename Tfp = Tret (*)(const void*, uint32_t, uint32_t,
                                      uint8_t*),
              typename T>
    static Tfp ptr(T& t)
    {
        fn<T>(&t);
        return (Tfp)lambda_ptr_exec<Tret, T>;
    }

    template <typename T>
    static void* fn(void* new_fn = nullptr)
    {
        static void* fn;
        if (new_fn != nullptr)
            fn = new_fn;
        return fn;
    }
};

std::optional<struct libcr51sign_validated_regions>
    Cr51SignValidatorIpml::validateDescriptor(struct libcr51sign_ctx* ctx)
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

    auto readBuf = [this](const void*, uint32_t offset, uint32_t count,
                          uint8_t* buf) {
        if (count == 0)
            return static_cast<int>(LIBCR51SIGN_SUCCESS);
        if (!buf)
            return static_cast<int>(LIBCR51SIGN_ERROR_RUNTIME_FAILURE);
        std::memcpy(buf, firmwareFileBuf.data() + offset, count);
        return static_cast<int>(LIBCR51SIGN_SUCCESS);
    };
    intf.read = Lambda::ptr(readBuf);

    intf.prod_to_dev_downgrade_allowed = []() {
        return ALLOW_PROD_TO_DEV_DOWNGRADE;
    };
    intf.is_production_mode = []() { return IS_PRODUCTION_MODE; };

    // TODO: Validate the non-static regions after all of the regions are signed
    // This only applies to clean image that is not read from the flash
    // directly.
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
