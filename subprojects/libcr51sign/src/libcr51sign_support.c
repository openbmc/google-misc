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
#include <libcr51sign/libcr51sign_support.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef USER_PRINT
#define CPRINTS(ctx, ...) fprintf(stderr, __VA_ARGS__)
#endif

    // @func hash_init get ready to compute a hash
    //
    // @param[in] ctx - context struct
    // @param[in] hash_type - type of hash function to use
    //
    // @return nonzero on error, zero on success

    int hash_init(const void* ctx, enum hash_type type)
    {
        struct libcr51sign_ctx* context = (struct libcr51sign_ctx*)ctx;
        struct hash_ctx* hash_context = (struct hash_ctx*)context->priv;
        hash_context->hash_type = type;
        if (type == HASH_SHA2_256) // SHA256_Init returns 1
            SHA256_Init(&hash_context->sha256_ctx);
        else if (type == HASH_SHA2_512)
            SHA512_Init(&hash_context->sha512_ctx);
        else
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;

        return LIBCR51SIGN_SUCCESS;
    }

    // @func hash_update add data to the hash
    //
    // @param[in] ctx - context struct
    // @param[in] buf - data to add to hash
    // @param[in] count - number of bytes of data to add
    //
    // @return nonzero on error, zero on success

    int hash_update(void* ctx, const uint8_t* data, size_t size)
    {
        if (size == 0)
            return LIBCR51SIGN_SUCCESS;
        struct libcr51sign_ctx* context = (struct libcr51sign_ctx*)ctx;
        struct hash_ctx* hash_context = (struct hash_ctx*)context->priv;

        if (hash_context->hash_type == HASH_SHA2_256) // SHA256_Update returns 1
            SHA256_Update(&hash_context->sha256_ctx, data, size);
        else if (hash_context->hash_type == HASH_SHA2_512)
            SHA512_Update(&hash_context->sha512_ctx, data, size);
        else
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;

        return LIBCR51SIGN_SUCCESS;
    }

    // @func hash_final finish hash calculation
    //
    // @param[in] ctx - context struct
    // @param[out] hash - buffer to write hash to (guaranteed to be big enough)
    //
    // @return nonzero on error, zero on success

    int hash_final(void* ctx, uint8_t* hash)
    {
        int rv;
        struct libcr51sign_ctx* context = (struct libcr51sign_ctx*)ctx;
        struct hash_ctx* hash_context = (struct hash_ctx*)context->priv;

        if (hash_context->hash_type == HASH_SHA2_256)
            rv = SHA256_Final(hash, &hash_context->sha256_ctx);
        else if (hash_context->hash_type == HASH_SHA2_512)
            rv = SHA512_Final(hash, &hash_context->sha512_ctx);
        else
            return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;

        if (rv)
            return LIBCR51SIGN_SUCCESS;
        else
            return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    // @func verify check that the signature is valid for given hashed data
    //
    // @param[in] ctx - context struct
    // @param[in] scheme - type of signature, hash, etc.
    // @param[in] sig - signature blob
    // @param[in] sig_len - length of signature in bytes
    // @param[in] data - pre-hashed data to verify
    // @param[in] data_len - length of hashed data in bytes
    //
    // verify_signature expects RSA public key file path in ctx->key_ring
    // @return nonzero on error, zero on success

    int verify_signature(const void* ctx, enum signature_scheme sig_scheme,
                         const uint8_t* sig, size_t sig_len,
                         const uint8_t* data, size_t data_len)
    {
        // By default returns error.
        int rv = LIBCR51SIGN_ERROR_INVALID_ARGUMENT;

        CPRINTS(ctx, "\n sig_len %zu sig: ", sig_len);
        for (size_t i = 0; i < sig_len; i++)
        {
            CPRINTS(ctx, "%x", sig[i]);
        }

        struct libcr51sign_ctx* lctx = (struct libcr51sign_ctx*)ctx;
        FILE* fp = fopen(lctx->keyring, "r");
        RSA *rsa = NULL, *pub_rsa = NULL;
        EVP_PKEY* pkey = NULL;
        BIO* bio = BIO_new(BIO_s_mem());
        if (!fp)
        {
            CPRINTS(ctx, "\n fopen failed: ");
            goto clean_up;
        }

        pkey = PEM_read_PUBKEY(fp, 0, 0, 0);
        if (!pkey)
        {
            CPRINTS(ctx, "\n Read public key failed: ");
            goto clean_up;
        }

        rsa = EVP_PKEY_get1_RSA(pkey);
        if (!rsa)
        {
            goto clean_up;
        }
        pub_rsa = RSAPublicKey_dup(rsa);
        if (!RSA_print(bio, pub_rsa, 2))
        {
            CPRINTS(ctx, "\n RSA print failed ");
        }
        if (!pub_rsa)
        {
            CPRINTS(ctx, "\n no pub rsa: ");
            goto clean_up;
        }
        CPRINTS(ctx, "\n public rsa \n");
        char buffer[1024];
        while (BIO_read(bio, buffer, sizeof(buffer) - 1) > 0)
        {
            CPRINTS(ctx, " %s", buffer);
        }
        enum hash_type hash_type;
        rv = get_hash_type_from_signature(sig_scheme, &hash_type);
        if (rv != LIBCR51SIGN_SUCCESS)
        {
            CPRINTS(ctx, "\n Invalid hash_type! \n");
            goto clean_up;
        }
        int hash_nid = -1;
        if (hash_type == HASH_SHA2_256)
        {
            hash_nid = NID_sha256;
        }
        else if (hash_type == HASH_SHA2_512)
        {
            hash_nid = NID_sha512;
        }
        else
        {
            rv = LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
            goto clean_up;
        }

        int ret = RSA_verify(hash_nid, data, data_len, sig, sig_len, pub_rsa);
        // OpenSSL RSA_verify returns 1 on success and 0 on failure
        if (!ret)
        {
            CPRINTS(ctx, "\n OPENSSL_ERROR: %s \n",
                    ERR_error_string(ERR_get_error(), NULL));
            rv = LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
            goto clean_up;
        }
        rv = LIBCR51SIGN_SUCCESS;
        CPRINTS(ctx, "\n sig: ");
        for (size_t i = 0; i < sig_len; i++)
        {
            CPRINTS(ctx, "%x", sig[i]);
        }

        CPRINTS(ctx, "\n data: ");
        for (size_t i = 0; i < data_len; i++)
        {
            CPRINTS(ctx, "%x", data[i]);
        }
        const unsigned rsa_size = RSA_size(pub_rsa);
        CPRINTS(ctx, "\n rsa size %d sig_len %d", rsa_size, (uint32_t)sig_len);

    clean_up:
        if (fp)
        {
            fclose(fp);
        }
        EVP_PKEY_free(pkey);
        RSA_free(rsa);
        RSA_free(pub_rsa);
        BIO_free(bio);
        return rv;
    }

#ifdef __cplusplus
} //  extern "C"
#endif
