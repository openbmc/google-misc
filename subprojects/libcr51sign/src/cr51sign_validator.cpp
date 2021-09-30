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

#include <fcntl.h>
#include <libcr51sign/cr51_image_descriptor.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_support.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcr51sign/cr51sign_validator.hpp>
#include <stdplus/print.hpp>
#include <stdplus/raw.hpp>

#include <format>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
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

std::span<const uint8_t>
    Cr51SignValidatorIpml::hashDescriptor(struct libcr51sign_ctx* ctx,
                                          std::span<std::byte> imageDescriptor)
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
            stdplus::println(stderr, "CR51 Hash type is not supported: type {}",
                             static_cast<int>(hashType));
            return {};
            break;
    }

    hash.resize(size);
    int ec = hash_init(ctx, hashType);
    if (ec)
    {
        stdplus::println(stderr, "CR51 Hash init error: ec{}", ec);
        return {};
    }

    ec = hash_update(ctx,
                     reinterpret_cast<const uint8_t*>(imageDescriptor.data()),
                     ctx->descriptor.descriptor_area_size);
    if (ec)
    {
        stdplus::println(stderr, "CR51 Hash update error: ec{}", ec);
        return {};
    }

    ec = hash_final(ctx, hash.data());
    if (ec)
    {
        stdplus::println(stderr, "CR51 Hash final error: ec{}", ec);
        return {};
    }

    return hash;
}

std::optional<struct libcr51sign_validated_regions>
    Cr51SignValidatorIpml::validateDescriptor(struct libcr51sign_ctx* ctx,
                                              struct libcr51sign_intf* intf)
{
    // Disabled stderr for all info messages
    fpos_t pos;

    // Save stderr location
    fgetpos(stderr, &pos);
    int fd = dup(fileno(stderr));

    // Redirect them to /dev/null
    if (!freopen("/dev/null", "w", stderr))
    {
        throw std::runtime_error("failed to redirect stderr to /dev/null");
    }

    struct libcr51sign_validated_regions imageRegions;
    enum libcr51sign_validation_failure_reason ec;

    /* Common functions are from the libcr51sign support header. */
    intf->hash_init = &hash_init;
    intf->hash_update = &hash_update;
    intf->hash_final = &hash_final;
    intf->verify_signature = &verify_signature;

    auto returnTrue = []() { return true; };
    auto returnFalse = []() { return false; };
    intf->prod_to_dev_downgrade_allowed = prodToDev ? returnTrue : returnFalse;
    intf->is_production_mode = productionMode ? returnTrue : returnFalse;

    // TODO: Validate the non-static regions after all of the regions are signed
    // This only applies to clean image that is not read from the flash
    // directly.
    ec = libcr51sign_validate(ctx, intf, &imageRegions);

    fflush(stderr);
    dup2(fd, fileno(stderr)); // restore the stderr
    close(fd);
    clearerr(stderr);

    // Move stderr to the normal location
    fsetpos(stderr, &pos);

    if (ec != LIBCR51SIGN_SUCCESS)
    {
        stdplus::println("CR51 Validate error: {}",
                         libcr51sign_errorcode_to_string(ec));
        return std::nullopt;
    }

    return imageRegions;
}
} // namespace cr51
} // namespace google
