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

#include <libcr51sign/cr51_image_descriptor.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LIBCR51SIGN_SHA256_DIGEST_SIZE 32
#define LIBCR51SIGN_SHA512_DIGEST_SIZE 64

#define LIBCR51SIGN_MAX_REGION_COUNT 16

// Currently RSA4096 (in bytes).
#define LIBCR51SIGN_MAX_SIGNATURE_SIZE 512

// LINT.IfChange(image_mauv_max_size_def)
#define IMAGE_MAUV_DATA_MAX_SIZE (128)
// LINT.ThenChange()

// State of the image.
enum libcr51sign_validation_state
{
    LIBCR51SIGN_IMAGE_UNSPECIFIED = 0,
    // The image fails at least one descriptor or region check.
    LIBCR51SIGN_IMAGE_INVALID = 1,
    // The image passes all descriptor and region checks. Note that this does
    // not mean that the image is valid for update. For example, the image may
    // not pass MAUV checks.
    LIBCR51SIGN_IMAGE_VALID = 2,
};

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
    // Invalid image region
    LIBCR51SIGN_ERROR_INVALID_REGION_INPUT = 16,
    LIBCR51SIGN_ERROR_INVALID_REGION_SIZE = 17,
    LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA = 18,
    LIBCR51SIGN_ERROR_RETRIEVING_STORED_IMAGE_MAUV_DATA = 19,
    LIBCR51SIGN_ERROR_STORING_NEW_IMAGE_MAUV_DATA = 20,
    LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_DOES_NOT_ALLOW_UPDATE_TO_PAYLOAD = 21,
    LIBCR51SIGN_ERROR_VALID_IMAGE_BUT_NEW_IMAGE_MAUV_DATA_NOT_STORED = 22,
    LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_EXPECTS_PAYLOAD_IMAGE_MAUV = 23,
    // Client did not find any stored MAUV in system
    LIBCR51SIGN_NO_STORED_MAUV_FOUND = 24,
    LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR_BLOBS = 25,
    LIBCR51SIGN_ERROR_MAX = 26,
};

struct libcr51sign_ctx
{
    // Expectations needed to validate an image. Users must set these fields
    // before calling libcr51sign_validate().
    uint32_t start_offset;                  // Absolute image start offset
    uint32_t end_offset;                    // Absolute image end offset
    enum image_family current_image_family; // Expected image family
    enum image_type current_image_type;     // Expected image type
    int keyring_len;     // keyring_len - number of keys in @a keyring
    const void* keyring; // keyring - array of pointers to public keys
    void* priv;          // opaque context data (used for hash state)

    // Data that is accessible if the image is valid after calling
    // libcr51sign_validate().
    enum libcr51sign_validation_state validation_state;
    size_t* valid_key; // valid_key - index of valid key
    // Note: `descriptor` needs to be the last member of this struct due to the
    // flexible array member in struct image_descriptor.
    struct image_descriptor descriptor; // Cr51 image descriptor.
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

    // Note this is a combination of an spi_nor_read() with spi_transaction()
    // It is the responsibility of the caller to synchronize with other
    // potential SPI clients / transactions. Collapsing the SPI stack results in
    // a 2x throughput improvement (~20s -> ~10s to verify an Indus image with
    // SHA256 HW acceleration).
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

    int (*verify_signature)(const void*, enum signature_scheme, const uint8_t*,
                            size_t, const uint8_t*, size_t);

    // @func verify check that if the prod to dev downgrade/ hardware allowlist
    // is allowed
    // @return  true: if allowed
    //          false: if not allowed
    // BMC would return always false or pass a NULL pointer
    // If NULL, treated as if the function always returns false.

    bool (*prod_to_dev_downgrade_allowed)();

    // @func returns true if the current firmware is running in production mode.
    // @return true: if in production mode
    //         false: if in any non-production mode

    bool (*is_production_mode)();

    // @func returns true if the descriptor image size is valid.
    bool (*image_size_valid)(size_t);

    // @func Retrieve MAUV data currently stored in the system
    // @param[in]  ctx - context struct
    // @param[out] current_image_mauv - Buffer to store the retrieved MAUV data
    // @param[out] current_image_mauv_size - Number of bytes retrieved and
    // stored
    //                                       in `current_image_mauv`
    // @param[in]  max_image_mauv_size - Maximum number of bytes to retrieve for
    //                                   MAUV data
    //
    // @return LIBCR51SIGN_SUCCESS: when MAUV is present in the system and
    //                              retrieved successfully
    //         LIBCR51SIGN_NO_STORED_MAUV_FOUND: when MAUV is not present in the
    //                                           system (we are trusting the
    //                                           client here to return this
    //                                           value truthfully)
    //         other non-zero values: any other error scenario (like read
    //         failure,
    //                                data corruption, etc.)
    int (*retrieve_stored_image_mauv_data)(const void*, uint8_t* const,
                                           uint32_t* const, const uint32_t);

    // @func Store new MAUV data in the system
    // @param[in]  ctx - context struct
    // @param[in]  new_image_mauv - Buffer containing new MAUV data to be stored
    // @param[in]  new_image_mauv_size - Size of MAUV data in `new_image_mauv`
    //                                   buffer
    //
    // @return LIBCR51SIGN_SUCCESS: when new MAUV data is stored successfully.
    //                              Non-zero value otherwise
    int (*store_new_image_mauv_data)(const void*, const uint8_t* const,
                                     const uint32_t);

    // @func trust descriptor hash
    // @param[in]  ctx - context struct
    // @param[in]  descriptor_hash - Buffer containing descriptor hash
    // @param[in]  descriptor_hash_size - Size of descriptor hash
    //
    // @return true: if the external key is trusted
    //         false: if the external key is not trusted
    bool (*trust_descriptor_hash)(const void*, const uint8_t*, size_t);

    // @func Trust key in the signature structure
    // @param[in]  ctx - context struct
    // @param[in]  scheme - signature scheme
    // @param[in]  signature_structure - signature structure
    // @param[in]  signature_structure_size - Size of signature structure in
    // bytes
    //
    // @return true: if the key in signature structure is trusted
    //         false: if the key in signature structure is not trusted
    bool (*trust_key_in_signature_structure)(
        void*, enum signature_scheme scheme, const void*, size_t);

    // @func Verify RSA signature with modulus and exponent
    // @param[in]  ctx - context struct
    // @param[in]  sig_scheme - signature scheme
    // @param[in]  modulus - modulus of the RSA key, MSB (big-endian)
    // @param[in]  modulus_len - length of modulus in bytes
    // @param[in]  exponent - exponent of the RSA key
    // @param[in]  sig - signature blob
    // @param[in]  sig_len - length of signature in bytes
    // @param[in]  digest - digest to verify
    // @param[in]  digest_len - digest size
    //
    // @return true: if the signature is verified
    //         false: otherwise
    bool (*verify_rsa_signature_with_modulus_and_exponent)(
        const void* ctx, enum signature_scheme scheme, const uint8_t* modulus,
        int modulus_len, uint32_t exponent, const uint8_t* sig, int sig_len,
        const uint8_t* digest, int digest_len);
};

struct libcr51sign_validated_regions
{
    uint32_t region_count;
    struct image_region image_regions[LIBCR51SIGN_MAX_REGION_COUNT];
};

// Check whether the signature on the image is valid.
// Validates the authenticity of an EEPROM image. Scans for & validates the
// signature on the image descriptor. If the descriptor validates, hashes the
// rest of the image to verify its integrity.
//
// @param[in] ctx - context which describes the image and holds opaque private
//                  data for the user of the library
// @param[in] intf - function pointers which interface to the current system
//                   and environment
// @param[out] image_regions - image_region pointer to an array for the output
//
// @return nonzero on error, zero on success

enum libcr51sign_validation_failure_reason libcr51sign_validate(
    struct libcr51sign_ctx* ctx, struct libcr51sign_intf* intf,
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

enum libcr51sign_validation_failure_reason get_hash_type_from_signature(
    enum signature_scheme scheme, enum hash_type* type);

#ifdef __cplusplus
} //  extern "C"
#endif

#endif // PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_H_
