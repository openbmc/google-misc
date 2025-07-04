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
#include <assert.h>
#include <libcr51sign/cr51_image_descriptor.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_internal.h>
#include <libcr51sign/libcr51sign_mauv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

// True of x is a power of two
#define POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))

// Maximum version supported. Major revisions are not backwards compatible.
#define MAX_MAJOR_VERSION 1

// Descriptor alignment on the external EEPROM.
#define DESCRIPTOR_ALIGNMENT (64 * 1024)

// SPS EEPROM sector size is 4KiB, since this is the smallest erasable size.
#define IMAGE_REGION_ALIGNMENT 4096

#define MAX_READ_SIZE 1024

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(t) (sizeof(t) / sizeof(t[0]))
#endif

// Values of SIGNATURE_OFFSET should be same for all sig types (2048,3072,4096)
#define SIGNATURE_OFFSET offsetof(struct signature_rsa3072_pkcs15, modulus)

#ifndef BUILD_ASSERT
#define BUILD_ASSERT(cond) ((void)sizeof(char[1 - 2 * !(cond)]))
#endif

// Returns the bytes size of keys used in the given signature_scheme.
// Return error if signature_scheme is invalid.
//
static failure_reason get_key_size(enum signature_scheme signature_scheme,
                                   uint16_t* key_size)
{
    switch (signature_scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
            *key_size = 256;
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA3072_PKCS15:
            *key_size = 384;
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA4096_PKCS15:
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            *key_size = 512;
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME;
    }
}

// Returns the hash_type for a given signature scheme
// Returns error if scheme is invalid.
failure_reason get_hash_type_from_signature(enum signature_scheme scheme,
                                            enum hash_type* type)
{
    switch (scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
        case SIGNATURE_RSA3072_PKCS15:
        case SIGNATURE_RSA4096_PKCS15:
            *type = HASH_SHA2_256;
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            *type = HASH_SHA2_512;
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME;
    }
}

// Check if the given hash_type is supported.
// Returns error if hash_type is not supported.
static failure_reason is_hash_type_supported(enum hash_type type)
{
    switch (type)
    {
        case HASH_SHA2_256:
        case HASH_SHA2_512:
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }
}

// Determines digest size for a given hash_type.
// Returns error if hash_type is not supported.
static failure_reason get_hash_digest_size(enum hash_type type, uint32_t* size)
{
    switch (type)
    {
        case HASH_SHA2_256:
            *size = LIBCR51SIGN_SHA256_DIGEST_SIZE;
            return LIBCR51SIGN_SUCCESS;
        case HASH_SHA2_512:
            *size = LIBCR51SIGN_SHA512_DIGEST_SIZE;
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }
}

// Determines hash struct size for a given hash_type.
// Returns error if hash_type is not supported.
static failure_reason get_hash_struct_size(enum hash_type type, uint32_t* size)
{
    switch (type)
    {
        case HASH_SHA2_256:
            *size = sizeof(struct hash_sha256);
            return LIBCR51SIGN_SUCCESS;
        case HASH_SHA2_512:
            *size = sizeof(struct hash_sha512);
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }
}

// Checks that:
//  - The signing key is trusted
//  - The target version is not denylisted
// If validating a staged update, also checks that:
//  - The target image family matches the current image family
//  - The image type transition is legal (i.e. dev -> *|| prod -> prod) or
//    alternatively that the hardware ID is allowlisted
// Assuming the caller has performed following:
// board_get_base_key_index();
// board_get_key_array
// Possible return codes:
// LIBCR51SIGN_SUCCESS = 0,
// LIBCR51SIGN_ERROR_RUNTIME_FAILURE = 1,
// LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR = 3,
// LIBCR51SIGN_ERROR_INVALID_IMAGE_FAMILY = 4,
// LIBCR51SIGN_ERROR_IMAGE_TYPE_DISALLOWED = 5,

static failure_reason validate_transition(const struct libcr51sign_ctx* ctx,
                                          const struct libcr51sign_intf* intf,
                                          uint32_t signature_struct_offset)
{
    BUILD_ASSERT((offsetof(struct signature_rsa2048_pkcs15, modulus) ==
                      SIGNATURE_OFFSET &&
                  offsetof(struct signature_rsa3072_pkcs15, modulus) ==
                      SIGNATURE_OFFSET &&
                  offsetof(struct signature_rsa4096_pkcs15, modulus) ==
                      SIGNATURE_OFFSET));

    // Read up to the modulus.
    enum
    {
        read_len = SIGNATURE_OFFSET
    };
    uint32_t buffer[read_len / sizeof(uint32_t)];
    int rv;
    rv = intf->read(ctx, signature_struct_offset, read_len, (uint8_t*)buffer);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read signature struct\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }
    if (*buffer != SIGNATURE_MAGIC)
    {
        CPRINTS(ctx, "%s: bad signature magic\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }

    if (ctx->descriptor.image_family != ctx->current_image_family &&
        ctx->descriptor.image_family != IMAGE_FAMILY_ALL &&
        ctx->current_image_family != IMAGE_FAMILY_ALL)
    {
        CPRINTS(ctx, "%s: invalid image family\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_FAMILY;
    }

    if (intf->is_production_mode == NULL)
    {
        CPRINTS(ctx, "%s: missing is_production_mode\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }
    if (intf->is_production_mode() && (ctx->descriptor.image_type == IMAGE_DEV))
    {
        CPRINTS(ctx, "%s: checking exemption allowlist\n", __FUNCTION__);

        // If function is NULL or if the function call return false, return
        // error
        if (intf->prod_to_dev_downgrade_allowed == NULL ||
            !intf->prod_to_dev_downgrade_allowed())
        {
            CPRINTS(ctx, "%s: illegal image type\n", __FUNCTION__);
            return LIBCR51SIGN_ERROR_DEV_DOWNGRADE_DISALLOWED;
        }
    }
    return LIBCR51SIGN_SUCCESS;
}

// If caller had provided read_and_hash_update call that, otherwise call read
// and then update.

static failure_reason read_and_hash_update(struct libcr51sign_ctx* ctx,
                                           const struct libcr51sign_intf* intf,
                                           uint32_t offset, uint32_t size)
{
    uint8_t read_buffer[MAX_READ_SIZE];
    int rv;
    uint32_t read_size;

    if (intf->read_and_hash_update)
    {
        rv = intf->read_and_hash_update(ctx, offset, size);
    }
    else
    {
        if (!intf->hash_update)
        {
            CPRINTS(ctx, "%s: missing hash_update\n", __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
        }
        do
        {
            read_size = size < MAX_READ_SIZE ? size : MAX_READ_SIZE;
            rv = intf->read(ctx, offset, read_size, read_buffer);
            if (rv != LIBCR51SIGN_SUCCESS)
            {
                return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
            }
            rv = intf->hash_update(ctx, read_buffer, read_size);
            if (rv != LIBCR51SIGN_SUCCESS)
            {
                return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
            }
            offset += read_size;
            size -= read_size;
        } while (size > 0);
    }
    return rv;
}

// Validates the image_region array, namely that:
//  - The regions are aligned, contiguous & exhaustive
//  - That the image descriptor resides in a static region
//
// If the array is consistent, proceeds to hash the static regions and
// validates the hash. d_offset is the absolute image descriptor offset

static failure_reason validate_payload_regions(
    struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
    uint32_t d_offset, struct libcr51sign_validated_regions* image_regions)
{
    // Allocate buffer to accommodate largest supported hash-type(SHA512)
    uint8_t magic_and_digest[MEMBER_SIZE(struct hash_sha512, hash_magic) +
                             LIBCR51SIGN_SHA512_DIGEST_SIZE];
    uint8_t dcrypto_digest[LIBCR51SIGN_SHA512_DIGEST_SIZE];
    uint32_t byte_count, region_count, image_size, hash_offset, digest_size;
    uint32_t i;
    uint32_t d_region_num = 0;
    int rv;
    struct image_region const* region;

    if (image_regions == NULL)
    {
        CPRINTS(ctx, "%s: Missing image region input\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_REGION_INPUT;
    }

    BUILD_ASSERT((MEMBER_SIZE(struct hash_sha256, hash_magic) ==
                  MEMBER_SIZE(struct hash_sha512, hash_magic)));
    image_size = ctx->descriptor.image_size;
    region_count = ctx->descriptor.region_count;
    hash_offset = d_offset + sizeof(struct image_descriptor) +
                  region_count * sizeof(struct image_region);
    // Read the image_region array.

    if (region_count > ARRAY_SIZE(image_regions->image_regions))
    {
        CPRINTS(ctx,
                "%s: ctx->descriptor.region_count is greater "
                "than LIBCR51SIGN_MAX_REGION_COUNT\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_REGION_SIZE;
    }

    rv = intf->read(ctx, d_offset + sizeof(struct image_descriptor),
                    region_count * sizeof(struct image_region),
                    (uint8_t*)&image_regions->image_regions);

    image_regions->region_count = region_count;

    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read region array\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    // Validate that the regions are contiguous & exhaustive.
    for (i = 0, byte_count = 0; i < region_count; i++)
    {
        region = image_regions->image_regions + i;

        CPRINTS(ctx, "%s: region #%d \"%s\" (%x - %x)\n", __FUNCTION__, i,
                (const char*)region->region_name, region->region_offset,
                region->region_offset + region->region_size);
        if ((region->region_offset % IMAGE_REGION_ALIGNMENT) != 0 ||
            (region->region_size % IMAGE_REGION_ALIGNMENT) != 0)
        {
            CPRINTS(ctx, "%s: regions must be sector aligned\n", __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
        }
        if (region->region_offset != byte_count ||
            region->region_size > image_size - byte_count)
        {
            CPRINTS(ctx, "%s: invalid region array\n", __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
        }
        byte_count += region->region_size;
        // The image descriptor must be part of a static region.
        if (d_offset >= region->region_offset && d_offset < byte_count)
        {
            d_region_num = i;
            CPRINTS(ctx, "%s: image descriptor in region %d\n", __FUNCTION__,
                    i);
            // The descriptor can't span regions.
            if ((ctx->descriptor.descriptor_area_size >
                 (byte_count - d_offset)) ||
                !(region->region_attributes & IMAGE_REGION_STATIC))
            {
                CPRINTS(ctx,
                        "%s: descriptor must reside in "
                        "static region\n",
                        __FUNCTION__);
                return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
            }
        }
    }
    if (byte_count != image_size)
    {
        CPRINTS(ctx, "%s: invalid image size\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }

    rv = get_hash_digest_size(ctx->descriptor.hash_type, &digest_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    rv = intf->read(ctx, hash_offset,
                    MEMBER_SIZE(struct hash_sha256, hash_magic) + digest_size,
                    magic_and_digest);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read hash from flash\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }
    if (*(uint32_t*)magic_and_digest != HASH_MAGIC)
    {
        CPRINTS(ctx, "%s: bad hash magic\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }
    rv = intf->hash_init(ctx, ctx->descriptor.hash_type);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: hash_init failed\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }
    for (i = 0; i < region_count; i++)
    {
        uint32_t hash_start, hash_size;
        region = image_regions->image_regions + i;

        if (!(region->region_attributes & IMAGE_REGION_STATIC))
        {
            continue;
        }
        hash_start = region->region_offset;
        hash_size = region->region_size;

        // Skip the descriptor.
        do
        {
            if (i == d_region_num)
            {
                hash_size = d_offset - hash_start;
                if (!hash_size)
                {
                    hash_start += ctx->descriptor.descriptor_area_size;
                    hash_size = (region->region_offset + region->region_size -
                                 hash_start);
                }
            }

            CPRINTS(ctx, "%s: hashing %s (%x - %x)\n", __FUNCTION__,
                    (const char*)region->region_name, hash_start,
                    hash_start + hash_size);
            // Read the image_region array.
            rv = read_and_hash_update(ctx, intf, hash_start, hash_size);
            if (rv != LIBCR51SIGN_SUCCESS)
            {
                return rv;
            }
            hash_start += hash_size;
        } while (hash_start != region->region_offset + region->region_size);
    }
    rv = intf->hash_final(ctx, (uint8_t*)dcrypto_digest);

    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    if (memcmp(magic_and_digest + MEMBER_SIZE(struct hash_sha256, hash_magic),
               dcrypto_digest, digest_size) != 0)
    {
        CPRINTS(ctx, "%s: invalid hash\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_HASH;
    }
    // Image is valid.
    return LIBCR51SIGN_SUCCESS;
}

// Create empty image_regions to pass to validate_payload_regions
// Support validate_payload_regions_helper to remove image_regions as a required
// input.

static failure_reason allocate_and_validate_payload_regions(
    struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
    uint32_t d_offset)
{
    struct libcr51sign_validated_regions image_regions;
    return validate_payload_regions(ctx, intf, d_offset, &image_regions);
}

// Wrapper around validate_payload_regions to allow nullptr for image_regions.
// Calls allocate_and_validate_payload_regions when image_regions is nullptr to
// create placer holder image_regions.

static failure_reason validate_payload_regions_helper(
    struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
    uint32_t d_offset, struct libcr51sign_validated_regions* image_regions)
{
    if (image_regions)
    {
        return validate_payload_regions(ctx, intf, d_offset, image_regions);
    }

    return allocate_and_validate_payload_regions(ctx, intf, d_offset);
}

// Check if the given signature_scheme is supported.
// Returns nonzero on error, zero on success

static failure_reason is_signature_scheme_supported(
    enum signature_scheme scheme)
{
    switch (scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
        case SIGNATURE_RSA3072_PKCS15:
        case SIGNATURE_RSA4096_PKCS15:
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME;
    }
}

// Returns size of signature struct size in |size|
// Returns nonzero on error, zero on success

static failure_reason get_signature_struct_size(enum signature_scheme scheme,
                                                uint32_t* size)
{
    switch (scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
            *size = sizeof(struct signature_rsa2048_pkcs15);
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA3072_PKCS15:
            *size = sizeof(struct signature_rsa3072_pkcs15);
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA4096_PKCS15:
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            *size = sizeof(struct signature_rsa4096_pkcs15);
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME;
    }
}

static failure_reason get_signature_field_offset(enum signature_scheme scheme,
                                                 uint32_t* offset)
{
    switch (scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
            *offset = offsetof(struct signature_rsa2048_pkcs15, signature);
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA3072_PKCS15:
            *offset = offsetof(struct signature_rsa3072_pkcs15, signature);
            return LIBCR51SIGN_SUCCESS;
        case SIGNATURE_RSA4096_PKCS15:
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            *offset = offsetof(struct signature_rsa4096_pkcs15, signature);
            return LIBCR51SIGN_SUCCESS;
        default:
            return LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME;
    }
}

__attribute__((nonnull)) static bool is_key_in_signature_struct_trusted(
    struct libcr51sign_ctx* ctx, const struct libcr51sign_intf* intf,
    enum signature_scheme scheme, uint32_t raw_signature_offset,
    void* signature_struct, uint32_t* signature_struct_size)
{
    if (!intf->trust_key_in_signature_structure)
    {
        CPRINTS(ctx, "%s: trust_key_in_signature_structure is not supported\n",
                __FUNCTION__);
        return false;
    }

    uint32_t signature_field_offset;
    int rv = get_signature_field_offset(scheme, &signature_field_offset);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return false;
    }

    if (signature_field_offset > raw_signature_offset)
    {
        CPRINTS(ctx,
                "%s: signature_field_offset (%d) is larger than "
                "raw_signature_offset (%d)\n",
                __FUNCTION__, signature_field_offset, raw_signature_offset);
        return false;
    }
    uint32_t signature_offset = raw_signature_offset - signature_field_offset;

    rv = get_signature_struct_size(scheme, signature_struct_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return false;
    }

    rv = intf->read(ctx, signature_offset, *signature_struct_size,
                    signature_struct);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read signature (status = %d)\n",
                __FUNCTION__, rv);
        return false;
    }

    return intf->trust_key_in_signature_structure(ctx, scheme, signature_struct,
                                                  *signature_struct_size);
}
// Validates the signature with verification key provided along with the
// signature if the key is trusted.

static bool validate_signature_with_key_in_signature_struct(
    struct libcr51sign_ctx* ctx, const struct libcr51sign_intf* intf,
    enum signature_scheme scheme, uint32_t raw_signature_offset,
    const uint8_t* digest, uint32_t digest_size)
{
    // pick the biggest signature struct size.
    uint8_t signature_struct[sizeof(struct signature_rsa4096_pkcs15)];
    uint32_t signature_struct_size = sizeof(signature_struct);
    if (!is_key_in_signature_struct_trusted(
            ctx, intf, scheme, raw_signature_offset, &signature_struct,
            &signature_struct_size))
    {
        CPRINTS(ctx, "%s: key in signature struct is not trusted\n",
                __FUNCTION__);
        return false;
    }
    if (!intf->verify_rsa_signature_with_modulus_and_exponent)
    {
        CPRINTS(
            ctx,
            "%s: verify_rsa_signature_with_modulus_and_exponent is not supported\n",
            __FUNCTION__);
        return false;
    }

    switch (scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
        {
            struct signature_rsa2048_pkcs15* sig =
                (struct signature_rsa2048_pkcs15*)signature_struct;
            return intf->verify_rsa_signature_with_modulus_and_exponent(
                ctx, scheme, sig->modulus, sizeof(sig->modulus), sig->exponent,
                sig->signature, sizeof(sig->signature), digest, digest_size);
        }
        break;
        case SIGNATURE_RSA3072_PKCS15:
        {
            struct signature_rsa3072_pkcs15* sig =
                (struct signature_rsa3072_pkcs15*)signature_struct;
            return intf->verify_rsa_signature_with_modulus_and_exponent(
                ctx, scheme, sig->modulus, sizeof(sig->modulus), sig->exponent,
                sig->signature, sizeof(sig->signature), digest, digest_size);
        }
        break;
        case SIGNATURE_RSA4096_PKCS15:
        case SIGNATURE_RSA4096_PKCS15_SHA512:
        {
            struct signature_rsa4096_pkcs15* sig =
                (struct signature_rsa4096_pkcs15*)signature_struct;
            return intf->verify_rsa_signature_with_modulus_and_exponent(
                ctx, scheme, sig->modulus, sizeof(sig->modulus), sig->exponent,
                sig->signature, sizeof(sig->signature), digest, digest_size);
        }
        break;
        default:
            CPRINTS(ctx, "%s: unsupported signature scheme %d\n", __FUNCTION__,
                    scheme);
            return false;
    }
}

// Validates the signature (of type scheme) read from "device" at
//"raw_signature_offset" with "public_key" over a SHA256/SHA512 digest of
// EEPROM area "data_offset:data_size".

static failure_reason validate_signature(
    struct libcr51sign_ctx* ctx, const struct libcr51sign_intf* intf,
    uint32_t data_offset, uint32_t data_size, enum signature_scheme scheme,
    uint32_t raw_signature_offset)
{
    uint8_t signature[LIBCR51SIGN_MAX_SIGNATURE_SIZE];
    uint16_t key_size;
    uint32_t digest_size;
    uint8_t dcrypto_digest[LIBCR51SIGN_SHA512_DIGEST_SIZE];
    int rv;
    enum hash_type hash_type;

    if (!intf->hash_init)
    {
        CPRINTS(ctx, "%s: missing hash_init\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }
    rv = get_hash_type_from_signature(scheme, &hash_type);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: hash_type from signature failed\n", __FUNCTION__);
        return rv;
    }
    rv = intf->hash_init(ctx, hash_type);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: hash_init failed\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }
    rv = read_and_hash_update(ctx, intf, data_offset, data_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: hash_update failed\n", __FUNCTION__);
        return rv;
    }
    if (!intf->hash_final)
    {
        CPRINTS(ctx, "%s: missing hash_final\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }
    rv = intf->hash_final(ctx, dcrypto_digest);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: hash_final failed (status = %d)\n", __FUNCTION__, rv);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    rv = get_hash_digest_size(hash_type, &digest_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    if (intf->trust_descriptor_hash)
    {
        if (intf->trust_descriptor_hash(ctx, dcrypto_digest, digest_size))
        {
            CPRINTS(ctx, "%s: descriptor hash trusted\n", __FUNCTION__);
            return LIBCR51SIGN_SUCCESS;
        }
    }

    rv = get_key_size(scheme, &key_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    rv = intf->read(ctx, raw_signature_offset, key_size, signature);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read signature (status = %d)\n",
                __FUNCTION__, rv);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    if (validate_signature_with_key_in_signature_struct(
            ctx, intf, scheme, raw_signature_offset, dcrypto_digest,
            digest_size))
    {
        CPRINTS(ctx, "%s: verification with external key succeeded\n",
                __FUNCTION__);
        return LIBCR51SIGN_SUCCESS;
    }

    if (!intf->verify_signature)
    {
        CPRINTS(ctx, "%s: missing verify_signature\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }

    rv = intf->verify_signature(ctx, scheme, signature, key_size,
                                dcrypto_digest, digest_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: verification failed (status = %d)\n", __FUNCTION__,
                rv);
        return LIBCR51SIGN_ERROR_INVALID_SIGNATURE;
    }
    CPRINTS(ctx, "%s: verification succeeded\n", __FUNCTION__);
    return LIBCR51SIGN_SUCCESS;
}

// Sanity checks the image descriptor & validates its signature.
// This function does not validate the image_region array or image hash.
//
//@param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
//@param[in] intf  function pointers which interface to the current system
// and environment
//@param offset  Absolute image descriptor flash offset.
//@param relative_offset  Image descriptor offset relative to image start.
//@param max_size Maximum size of the flash space in bytes.
//@param[out] payload_blob_offset  Absolute offset of BLOB data in image
//                                 descriptor (if BLOB data is present)
static failure_reason validate_descriptor(
    struct libcr51sign_ctx* ctx, const struct libcr51sign_intf* intf,
    uint32_t offset, uint32_t relative_offset, uint32_t max_size,
    uint32_t* const restrict payload_blob_offset)
{
    uint32_t max_descriptor_size, signed_size, signature_scheme,
        signature_offset;
    uint32_t signature_struct_offset, signature_struct_size, hash_struct_size;
    int rv;

    max_descriptor_size = max_size - relative_offset;
    if (max_size < relative_offset ||
        max_descriptor_size < sizeof(struct image_descriptor))
    {
        CPRINTS(ctx, "%s: invalid arguments\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }

    rv = intf->read(ctx, offset, sizeof(ctx->descriptor),
                    (uint8_t*)&ctx->descriptor);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: failed to read descriptor\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }
    if (ctx->descriptor.descriptor_magic != DESCRIPTOR_MAGIC ||
        ctx->descriptor.descriptor_offset != relative_offset ||
        ctx->descriptor.region_count == 0 ||
        ctx->descriptor.descriptor_area_size > max_descriptor_size ||
        ctx->descriptor.image_size > max_size)
    {
        CPRINTS(ctx, "%s: invalid descriptor\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }
    if (intf->image_size_valid == NULL)
    {
        // Preserve original behavior of requiring exact image_size match if no
        // operator is provided.
        if (ctx->descriptor.image_size != max_size)
        {
            CPRINTS(ctx, "%s: invalid image size\n", __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
        }
    }
    else if (!intf->image_size_valid(ctx->descriptor.image_size))
    {
        CPRINTS(ctx, "%s: invalid image size\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }
    if (ctx->descriptor.image_type != IMAGE_DEV &&
        ctx->descriptor.image_type != IMAGE_PROD &&
        ctx->descriptor.image_type != IMAGE_BREAKOUT &&
        ctx->descriptor.image_type != IMAGE_TEST &&
        ctx->descriptor.image_type != IMAGE_UNSIGNED_INTEGRITY)
    {
        CPRINTS(ctx, "%s: bad image type\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }
    // Although the image_descriptor struct supports unauthenticated
    // images, Haven will not allow it.
    // Haven only supports SHA256 + RSA2048/RSA3072_PKCS15 currently.

    signature_scheme = ctx->descriptor.signature_scheme;

    rv = is_signature_scheme_supported(signature_scheme);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }
    rv = is_hash_type_supported(ctx->descriptor.hash_type);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: invalid hash type\n", __FUNCTION__);
        return rv;
    }
    if (ctx->descriptor.descriptor_major > MAX_MAJOR_VERSION ||
        ctx->descriptor.region_count > LIBCR51SIGN_MAX_REGION_COUNT)
    {
        CPRINTS(ctx, "%s: unsupported descriptor\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_UNSUPPORTED_DESCRIPTOR;
    }
    rv = get_signature_struct_size(signature_scheme, &signature_struct_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    // Compute the size of the signed portion of the image descriptor.
    signed_size = sizeof(struct image_descriptor) +
                  ctx->descriptor.region_count * sizeof(struct image_region);
    rv = get_hash_struct_size(ctx->descriptor.hash_type, &hash_struct_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }
    signed_size += hash_struct_size;
    if (ctx->descriptor.denylist_size)
    {
        signed_size += sizeof(struct denylist);
        signed_size += ctx->descriptor.denylist_size *
                       sizeof(struct denylist_record);
    }
    if (ctx->descriptor.blob_size)
    {
        *payload_blob_offset = offset + signed_size;
        signed_size += sizeof(struct blob);
        // Previous additions are guaranteed not to overflow.
        if ((ctx->descriptor.blob_size >
             ctx->descriptor.descriptor_area_size - signed_size) ||
            // Sanity check blob size
            (ctx->descriptor.blob_size < sizeof(struct blob_data)))
        {
            CPRINTS(ctx, "%s: invalid blob size (0x%x)\n", __FUNCTION__,
                    ctx->descriptor.blob_size);
            return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
        }
        signed_size += ctx->descriptor.blob_size;
    }
    if (signature_struct_size >
        ctx->descriptor.descriptor_area_size - signed_size)
    {
        CPRINTS(ctx,
                "%s: invalid descriptor area size "
                "(expected = 0x%x, actual = 0x%x)\n",
                __FUNCTION__, ctx->descriptor.descriptor_area_size,
                signed_size + signature_struct_size);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }
    signature_struct_offset = signed_size;
    // Omit the actual signature.
    rv = get_signature_field_offset(signature_scheme, &signature_offset);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }
    signed_size += signature_offset;

    // Lookup key & validate transition.
    rv = validate_transition(ctx, intf, offset + signature_struct_offset);

    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }
    return validate_signature(ctx, intf, offset, signed_size, signature_scheme,
                              offset + signed_size);
}

// Scans the external EEPROM for a magic value at "alignment" boundaries.
//
//@param device  Handle to the external EEPROM.
//@param magic   8-byte pattern to search for.
//@param start_offset  Offset to begin searching at.
//@param limit   Exclusive address (e.g. EEPROM size).
//@param alignment   Alignment boundaries (POW2) to search on.
//@param header_offset   Location to place the new header offset.
//@return LIBCR51SIGN_SUCCESS (or non-zero on error).

static int scan_for_magic_8(struct libcr51sign_ctx* ctx,
                            const struct libcr51sign_intf* intf, uint64_t magic,
                            uint32_t start_offset, uint32_t limit,
                            uint32_t alignment, uint32_t* header_offset)
{
    uint64_t read_data;
    uint32_t offset;
    int rv;

    if (limit <= start_offset || limit > ctx->end_offset ||
        limit < sizeof(magic) || !POWER_OF_TWO(alignment))
    {
        return LIBCR51SIGN_ERROR_INVALID_ARGUMENT;
    }

    if (!intf->read)
    {
        CPRINTS(ctx, "%s: missing intf->read\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }
    // Align start_offset to the next valid boundary.
    start_offset = ((start_offset - 1) & ~(alignment - 1)) + alignment;
    for (offset = start_offset; offset < limit - sizeof(magic);
         offset += alignment)
    {
        rv = intf->read(ctx, offset, sizeof(read_data), (uint8_t*)&read_data);
        if (rv != LIBCR51SIGN_SUCCESS)
        {
            return rv;
        }
        if (read_data == magic)
        {
            if (header_offset)
            {
                *header_offset = offset;
            }
            return LIBCR51SIGN_SUCCESS;
        }
    }
    // Failed to locate magic.
    return LIBCR51SIGN_ERROR_FAILED_TO_LOCATE_MAGIC;
}

// Check whether the signature on the image is valid.
// Validates the authenticity of an EEPROM image. Scans for & validates the
// signature on the image descriptor. If the descriptor validates, hashes the
// rest of the image to verify its integrity.
//
// @param[in] ctx - context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf - function pointers which interface to the current system
//                  and environment
// @param[out] image_regions - image_region pointer to an array for the output
//
// @return nonzero on error, zero on success

failure_reason libcr51sign_validate(
    struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
    struct libcr51sign_validated_regions* image_regions)
{
    int rv, rv_first_desc = LIBCR51SIGN_SUCCESS;
    uint32_t descriptor_offset;
    uint32_t payload_blob_offset = 0;

    if (!ctx)
    {
        CPRINTS(ctx, "%s: Missing context\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_CONTEXT;
    }
    if (!intf)
    {
        CPRINTS(ctx, "%s: Missing interface\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }

    ctx->validation_state = LIBCR51SIGN_IMAGE_INVALID;

    rv = scan_for_magic_8(ctx, intf, DESCRIPTOR_MAGIC, ctx->start_offset,
                          ctx->end_offset, DESCRIPTOR_ALIGNMENT,
                          &descriptor_offset);
    while (rv == LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: potential image descriptor found @%x\n", __FUNCTION__,
                descriptor_offset);
        // Validation is split into 3 functions to minimize stack usage.

        rv = validate_descriptor(
            ctx, intf, descriptor_offset, descriptor_offset - ctx->start_offset,
            ctx->end_offset - ctx->start_offset, &payload_blob_offset);
        if (rv != LIBCR51SIGN_SUCCESS)
        {
            CPRINTS(ctx, "%s: validate_descriptor() failed ec%d\n",
                    __FUNCTION__, rv);
        }
        else
        {
            rv = validate_payload_regions_helper(ctx, intf, descriptor_offset,
                                                 image_regions);
            if (rv != LIBCR51SIGN_SUCCESS)
            {
                CPRINTS(ctx, "%s: validate_payload_regions() failed ec%d\n",
                        __FUNCTION__, rv);
            }
            else if (ctx->descriptor.image_type == IMAGE_PROD)
            {
                ctx->validation_state = LIBCR51SIGN_IMAGE_VALID;
                // Lookup and validate payload Image MAUV against Image MAUV
                // stored in the system after checking signature to ensure
                // offsets and sizes are not tampered with. Also, do this after
                // hash calculation for payload regions to ensure that stored
                // Image MAUV is updated (if necessary) as close to the end of
                // payload validation as possible
                rv = validate_payload_image_mauv(ctx, intf, payload_blob_offset,
                                                 ctx->descriptor.blob_size);
                if (rv == LIBCR51SIGN_SUCCESS)
                {
                    CPRINTS(ctx,
                            "%s: Payload Image MAUV validation successful\n",
                            __FUNCTION__);
                    return rv;
                }
                if (rv == LIBCR51SIGN_ERROR_STORING_NEW_IMAGE_MAUV_DATA)
                {
                    CPRINTS(
                        ctx,
                        "%s: Payload validation succeeded, but Image MAUV validation "
                        "failed\n",
                        __FUNCTION__);
                    return LIBCR51SIGN_ERROR_VALID_IMAGE_BUT_NEW_IMAGE_MAUV_DATA_NOT_STORED;
                }
                CPRINTS(ctx, "%s: Payload Image MAUV validation failed\n",
                        __FUNCTION__);
                // In practice, we expect only 1 valid image descriptor in
                // payload. If Image MAUV check fails for the payload after
                // validating the image descriptor, do not try validating other
                // image descriptors
                return rv;
            }
            else
            {
                ctx->validation_state = LIBCR51SIGN_IMAGE_VALID;
                return rv;
            }
        }

        // Store the first desc fail reason if any
        if (rv != LIBCR51SIGN_SUCCESS && rv_first_desc == LIBCR51SIGN_SUCCESS)
            rv_first_desc = rv;

        // scan_for_magic_8() will round up to the next aligned boundary.
        descriptor_offset++;
        rv = scan_for_magic_8(ctx, intf, DESCRIPTOR_MAGIC, descriptor_offset,
                              ctx->end_offset, DESCRIPTOR_ALIGNMENT,
                              &descriptor_offset);
    }
    CPRINTS(ctx, "%s: failed to validate image ec%d\n", __FUNCTION__, rv);
    // If desc validation failed for some reason then return that reason
    if (rv_first_desc != LIBCR51SIGN_SUCCESS)
        return rv_first_desc;

    return rv;
}

// @func to returns the libcr51sign error code as a string
// @param[in] ec - Error code
// @return error code in string format

const char* libcr51sign_errorcode_to_string(failure_reason ec)
{
    switch (ec)
    {
        case LIBCR51SIGN_SUCCESS:
            return "Success";
        case LIBCR51SIGN_ERROR_RUNTIME_FAILURE:
            return "Runtime Error Failure";
        case LIBCR51SIGN_ERROR_UNSUPPORTED_DESCRIPTOR:
            return "Unsupported descriptor";
        case LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR:
            return "Invalid descriptor";
        case LIBCR51SIGN_ERROR_INVALID_IMAGE_FAMILY:
            return "Invalid image family";
        case LIBCR51SIGN_ERROR_IMAGE_TYPE_DISALLOWED:
            return "Image type disallowed";
        case LIBCR51SIGN_ERROR_DEV_DOWNGRADE_DISALLOWED:
            return "Dev downgrade disallowed";
        case LIBCR51SIGN_ERROR_UNTRUSTED_KEY:
            return "Untrusted key";
        case LIBCR51SIGN_ERROR_INVALID_SIGNATURE:
            return "Invalid signature";
        case LIBCR51SIGN_ERROR_INVALID_HASH:
            return "Invalid hash";
        case LIBCR51SIGN_ERROR_INVALID_HASH_TYPE:
            return "Invalid hash type";
        case LIBCR51SIGN_ERROR_INVALID_ARGUMENT:
            return "Invalid Argument";
        case LIBCR51SIGN_ERROR_FAILED_TO_LOCATE_MAGIC:
            return "Failed to locate descriptor";
        case LIBCR51SIGN_ERROR_INVALID_CONTEXT:
            return "Invalid context";
        case LIBCR51SIGN_ERROR_INVALID_INTERFACE:
            return "Invalid interface";
        case LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME:
            return "Invalid signature scheme";
        case LIBCR51SIGN_ERROR_INVALID_REGION_INPUT:
            return "Invalid image region input";
        case LIBCR51SIGN_ERROR_INVALID_REGION_SIZE:
            return "Invalid image region size";
        case LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA:
            return "Invalid Image MAUV data";
        case LIBCR51SIGN_ERROR_RETRIEVING_STORED_IMAGE_MAUV_DATA:
            return "Failed to retrieve Image MAUV data stored in system";
        case LIBCR51SIGN_ERROR_STORING_NEW_IMAGE_MAUV_DATA:
            return "Failed to store Image MAUV data from payload image into system";
        case LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_DOES_NOT_ALLOW_UPDATE_TO_PAYLOAD:
            return "Image MAUV stored in system does not allow payload "
                   "update";
        case LIBCR51SIGN_ERROR_VALID_IMAGE_BUT_NEW_IMAGE_MAUV_DATA_NOT_STORED:
            return "Payload image is valid for update but failed to store new Image "
                   "MAUV in system";
        case LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_EXPECTS_PAYLOAD_IMAGE_MAUV:
            return "Image MAUV is expected to be present in payload when stored "
                   "Image MAUV is present in the system";
        case LIBCR51SIGN_NO_STORED_MAUV_FOUND:
            return "Client did not find any MAUV data stored in the system";
        case LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR_BLOBS:
            return "Invalid descriptor blobs";
        default:
            return "Unknown error";
    }
}

#ifdef __cplusplus
} //  extern "C"
#endif
