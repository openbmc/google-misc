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
#include <fmt/format.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_support.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcr51sign/cr51sign_validator.hpp>

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

std::span<const uint8_t>
    Cr51SignValidatorIpml::hashDescriptor(struct libcr51sign_ctx* ctx,
                                          std::span<const uint8_t> firmwareBuf)
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

    hash.resize(size);
    int ec = hash_init(ctx, hashType);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash init error: ec{}", ec);
        return {};
    }

    ec =
        hash_update(ctx, firmwareBuf.data() + ctx->descriptor.descriptor_offset,
                    ctx->descriptor.descriptor_area_size);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash update error: ec{}", ec);
        return {};
    }

    ec = hash_final(ctx, hash.data());
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
    Cr51SignValidatorIpml::validateDescriptor(
        struct libcr51sign_ctx* ctx, std::span<const uint8_t> firmwareBuf)
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

    auto readBuf = [firmwareBuf](const void*, uint32_t offset, uint32_t count,
                                 uint8_t* buf) {
        if (count == 0)
            return static_cast<int>(LIBCR51SIGN_SUCCESS);
        if (!buf)
            return static_cast<int>(LIBCR51SIGN_ERROR_RUNTIME_FAILURE);
        std::memcpy(buf, firmwareBuf.data() + offset, count);
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
        fmt::print(stderr, "CR51 Validate error: {}\n",
                   libcr51sign_errorcode_to_string(ec));
        return std::nullopt;
    }

    return imageRegions;
}

} // namespace cr51
} // namespace google
