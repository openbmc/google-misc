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
#ifndef PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_SUPPORT_H_
#define PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_SUPPORT_H_

#include <openssl/sha.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct hash_ctx
{
    enum hash_type hash_type;
    union
    {
        SHA256_CTX sha256_ctx;
        SHA512_CTX sha512_ctx;
    };
};

// @func hash_init get ready to compute a hash
//
// @param[in] ctx - context struct
// @param[in] hash_type - type of hash function to use
//
// @return nonzero on error, zero on success

int hash_init(const void* ctx, enum hash_type type);

// @func hash_update add data to the hash
//
// @param[in] ctx - context struct
// @param[in] buf - data to add to hash
// @param[in] count - number of bytes of data to add
//
// @return nonzero on error, zero on success

int hash_update(void* ctx, const uint8_t* data, size_t size);

// @func hash_final finish hash calculation
//
// @param[in] ctx - context struct
// @param[out] hash - buffer to write hash to (guaranteed to be big enough)
//
// @return nonzero on error, zero on success

int hash_final(void* ctx, uint8_t* hash);

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

int verify_signature(const void* ctx, enum signature_scheme sig_scheme,
                     const uint8_t* sig, size_t sig_len, const uint8_t* data,
                     size_t data_len);

#ifdef __cplusplus
} //  extern "C"
#endif
#endif // PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_SUPPORT_H_
