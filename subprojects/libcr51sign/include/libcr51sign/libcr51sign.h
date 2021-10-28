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
#ifndef PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_H_
#define PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_H_

#include "cr51_image_descriptor.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LIBCR51SIGN_SHA256_DIGEST_SIZE 32
#define LIBCR51SIGN_SHA512_DIGEST_SIZE 64

#define LIBCR51SIGN_MAX_REGION_COUNT 16

// Currently RSA4096 (in bytes).
#define LIBCR51SIGN_MAX_SIGNATURE_SIZE 512

    // List of common error codes that can be returned
    enum libcr51sign_validation_failure_reason
    {
        // All PayloadRegionState fields are valid & authenticated.
        LIBCR51SIGN_SUCCESS = 0,

        // Descriptor sanity check failed. None of the following
        // PayloadRegionState fields are valid/populated.
        LIBCR51SIGN_ERROR_RUNTIME_FAILURE = 1,
        LIBCR51SIGN_ERROR_UNSUPPORTED_DESCRIPTOR = 2,
        LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR = 3,

        // All fields are populated but may not be authentic.
        LIBCR51SIGN_ERROR_INVALID_IMAGE_FAMILY = 4,
        LIBCR51SIGN_ERROR_IMAGE_TYPE_DISALLOWED = 5,
        LIBCR51SIGN_ERROR_DEV_DOWNGRADE_DISALLOWED = 6,
        LIBCR51SIGN_ERROR_UNTRUSTED_KEY = 7,
        LIBCR51SIGN_ERROR_INVALID_SIGNATURE = 8,
        LIBCR51SIGN_ERROR_INVALID_HASH = 9,
        LIBCR51SIGN_ERROR_INVALID_HASH_TYPE = 10,
        // Invalid argument
        LIBCR51SIGN_ERROR_INVALID_ARGUMENT = 11,
        LIBCR51SIGN_ERROR_FAILED_TO_LOCATE_MAGIC = 12,
        LIBCR51SIGN_ERROR_INVALID_CONTEXT = 13,
        LIBCR51SIGN_ERROR_INVALID_INTERFACE = 14,
        LIBCR51SIGN_ERROR_INVALID_SIG_SCHEME = 15,
        LIBCR51SIGN_ERROR_MAX = 16,
        // Invalid image region
        LIBCR51SIGN_ERROR_INVALID_REGION_INPUT = 17,
        LIBCR51SIGN_ERROR_INVALID_REGION_SIZE = 18,
    };

    struct libcr51sign_ctx
    {
        // Absolute image start offset
        uint32_t start_offset;
        // Absolute image end offset
        uint32_t end_offset;
        size_t block_size;
        enum image_family current_image_family;
        enum image_type current_image_type;
        // keyring_len - number of keys in @a keyring
        int keyring_len;
        // valid_key - index of valid key on success
        size_t* valid_key;
        // keyring - array of pointers to public keys
        const void* keyring;
        void* priv;
        struct image_descriptor descriptor;
    };

    struct libcr51sign_intf
    {
        // @func read read data from the image into a buffer
        //
        // @param[in] ctx - context struct
        // @param[in] offset - bytes to seek into the image before reading
        // @param[in] count - number of bytes to read
        // @param[out] buf - pointer to buffer where result will be written
        //
        // @return nonzero on error, zero on success

        int (*read)(const void*, uint32_t, uint32_t, uint8_t*);

        // @func hash_init get ready to compute a hash
        //
        // @param[in] ctx - context struct
        // @param[in] hash_type - type of hash function to use
        //
        // @return nonzero on error, zero on success

        int (*hash_init)(const void*, enum hash_type);

        // @func hash_update add data to the hash
        //
        // @param[in] ctx - context struct
        // @param[in] buf - data to add to hash
        // @param[in] count - number of bytes of data to add
        // @param[in] hash_type - type of hash function to use
        //
        // @return nonzero on error, zero on success

        int (*hash_update)(void*, const uint8_t*, size_t);

        // Note this is a combination of an spi_nor_read() with
        // spi_transaction() It is the responsibility of the caller to
        // synchronize with other potential SPI clients / transactions.
        // Collapsing the SPI stack results in a 2x throughput improvement (~20s
        // -> ~10s to verify an Indus image with SHA256 HW acceleration).
        //
        // The caller is responsible for calling DCRYPTO_init()/HASH_final().

        int (*read_and_hash_update)(void* ctx, uint32_t offset, uint32_t size);

        // @func hash_final finish hash calculation
        //
        // @param[in] ctx - context struct
        // @param[out] hash - buffer to write hash to
        // @param[in] hash_type - type of hash function to use
        //
        // @return nonzero on error, zero on success

        int (*hash_final)(void*, uint8_t*);

        // @func verify check that the signature is valid for given hashed data
        //
        // @param[in] ctx - context struct
        // @param[in] scheme - type of signature, hash, etc.
        // @param[in] sig - signature blob
        // @param[in] sig_len - length of signature in bytes
        // @param[in] data - pre-hashed data to verify
        // @param[in] data_len - length of hashed data in bytes
        //
        // @return nonzero on error, zero on success

        int (*verify_signature)(const void*, enum signature_scheme,
                                const uint8_t*, size_t, const uint8_t*, size_t);

        // @func verify check that if the prod to dev downgrade/ hardware
        // allowlist is allowed
        // @return  true: if allowed
        //          false: if not allowed
        // BMC would return always false or pass a NULL pointer
        // If NULL, treated as if the function always returns false.

        bool (*prod_to_dev_downgrade_allowed)();

        // @func returns true if the current firmware is running in production
        // mode.
        // @return true: if in production mode
        //         false: if in any non-production mode

        bool (*is_production_mode)();
    };

    struct libcr51sign_validated_regions
    {
        uint32_t region_count;
        struct image_region image_regions[LIBCR51SIGN_MAX_REGION_COUNT];
    };

    // Check whether the signature on the image is valid.
    // Validates the authenticity of an EEPROM image. Scans for & validates the
    // signature on the image descriptor. If the descriptor validates, hashes
    // the rest of the image to verify its integrity.
    //
    // @param[in] ctx - context which describes the image and holds opaque
    // private
    //                  data for the user of the library
    // @param[in] intf - function pointers which interface to the current system
    //                   and environment
    // @param[out] image_regions - image_region pointer to an array for the
    // output
    //
    // @return nonzero on error, zero on success

    enum libcr51sign_validation_failure_reason libcr51sign_validate(
        const struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
        struct libcr51sign_validated_regions* image_regions);

    // Function to convert error code to string format
    // @param[in] ec - error code
    // @return error code in string format

    const char* libcr51sign_errorcode_to_string(
        enum libcr51sign_validation_failure_reason ec);

    // Returns the hash_type for a given signature scheme
    // @param[in] scheme - signature scheme
    // @param[out] type - hash_type supported by given signature_scheme
    //
    // @return nonzero on error, zero on success

    enum libcr51sign_validation_failure_reason
        get_hash_type_from_signature(enum signature_scheme scheme,
                                     enum hash_type* type);

#ifdef __cplusplus
} //  extern "C"
#endif

#endif // PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_H_
