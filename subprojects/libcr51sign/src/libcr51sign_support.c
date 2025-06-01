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
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <stdint.h>
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
    if (type == HASH_SHA2_256)
    { // SHA256_Init returns 1
        SHA256_Init(&hash_context->sha256_ctx);
    }
    else if (type == HASH_SHA2_512)
    {
        SHA512_Init(&hash_context->sha512_ctx);
    }
    else
    {
        return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }

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

    if (hash_context->hash_type == HASH_SHA2_256)
    { // SHA256_Update returns 1
        SHA256_Update(&hash_context->sha256_ctx, data, size);
    }
    else if (hash_context->hash_type == HASH_SHA2_512)
    {
        SHA512_Update(&hash_context->sha512_ctx, data, size);
    }
    else
    {
        return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }

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
    {
        rv = SHA256_Final(hash, &hash_context->sha256_ctx);
    }
    else if (hash_context->hash_type == HASH_SHA2_512)
    {
        rv = SHA512_Final(hash, &hash_context->sha512_ctx);
    }
    else
    {
        return LIBCR51SIGN_ERROR_INVALID_HASH_TYPE;
    }

    if (rv)
    {
        return LIBCR51SIGN_SUCCESS;
    }

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
                     const uint8_t* sig, size_t sig_len, const uint8_t* data,
                     size_t data_len)
{
    // By default returns error.
    int rv = LIBCR51SIGN_ERROR_INVALID_ARGUMENT;

    CPRINTS(ctx, "sig_len %zu sig: ", sig_len);
    for (size_t i = 0; i < sig_len; i++)
    {
        CPRINTS(ctx, "%x", sig[i]);
    }
    CPRINTS(ctx, "\n");

    struct libcr51sign_ctx* lctx = (struct libcr51sign_ctx*)ctx;
    FILE* fp = fopen(lctx->keyring, "r");
    RSA *rsa = NULL, *pub_rsa = NULL;
    EVP_PKEY* pkey = NULL;
    BIO* bio = BIO_new(BIO_s_mem());
    if (!fp)
    {
        CPRINTS(ctx, "fopen failed\n");
        goto clean_up;
    }

    pkey = PEM_read_PUBKEY(fp, 0, 0, 0);
    if (!pkey)
    {
        CPRINTS(ctx, "Read public key failed\n");
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
        CPRINTS(ctx, "RSA print failed\n");
    }
    if (!pub_rsa)
    {
        CPRINTS(ctx, "no pub RSA\n");
        goto clean_up;
    }
    CPRINTS(ctx, "public RSA\n");
    char buffer[1024] = {};
    while (BIO_read(bio, buffer, sizeof(buffer) - 1) > 0)
    {
        CPRINTS(ctx, " %s", buffer);
    }
    enum hash_type hash_type;
    rv = get_hash_type_from_signature(sig_scheme, &hash_type);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "Invalid hash_type!\n");
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
        CPRINTS(ctx, "OPENSSL_ERROR: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        rv = LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
        goto clean_up;
    }
    rv = LIBCR51SIGN_SUCCESS;
    CPRINTS(ctx, "sig: ");
    for (size_t i = 0; i < sig_len; i++)
    {
        CPRINTS(ctx, "%x", sig[i]);
    }
    CPRINTS(ctx, "\n");

    CPRINTS(ctx, "data: ");
    for (size_t i = 0; i < data_len; i++)
    {
        CPRINTS(ctx, "%x", data[i]);
    }
    CPRINTS(ctx, "\n");

    const unsigned rsa_size = RSA_size(pub_rsa);
    CPRINTS(ctx, "rsa size %d sig_len %d\n", rsa_size, (uint32_t)sig_len);

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
__attribute__((nonnull)) bool verify_rsa_signature_with_modulus_and_exponent(
    const void* ctx, enum signature_scheme sig_scheme, const uint8_t* modulus,
    int modulus_len, uint32_t exponent, const uint8_t* sig, int sig_len,
    const uint8_t* digest, int digest_len)
{
    RSA* rsa = NULL;
    BIGNUM* n = NULL;
    BIGNUM* e = NULL;
    int ret = 0;
    int hash_nid = NID_undef;
    int expected_modulus_bits = 0;
    int expected_digest_len = 0;

    CPRINTS(ctx, "%s: sig_scheme = %d\n", __FUNCTION__, sig_scheme);
    // Determine hash NID and expected modulus size based on signature_scheme
    switch (sig_scheme)
    {
        case SIGNATURE_RSA2048_PKCS15:
            expected_modulus_bits = 2048;
            hash_nid = NID_sha256;
            expected_digest_len = SHA256_DIGEST_LENGTH;
            break;
        case SIGNATURE_RSA3072_PKCS15:
            expected_modulus_bits = 3072;
            hash_nid = NID_sha256;
            expected_digest_len = SHA256_DIGEST_LENGTH;
            break;
        case SIGNATURE_RSA4096_PKCS15:
            expected_modulus_bits = 4096;
            hash_nid = NID_sha256;
            expected_digest_len = SHA256_DIGEST_LENGTH;
            break;
        case SIGNATURE_RSA4096_PKCS15_SHA512:
            expected_modulus_bits = 4096;
            hash_nid = NID_sha512;
            expected_digest_len = SHA512_DIGEST_LENGTH;
            break;
        default:
            CPRINTS(ctx, "%s: Unsupported signature scheme.\n", __FUNCTION__);
            return false;
    }

    // Input validation: Check digest length
    if (digest_len != expected_digest_len)
    {
        CPRINTS(
            ctx,
            "%s: Mismatch in expected digest length (%d) and actual (%d).\n",
            __FUNCTION__, expected_digest_len, digest_len);
        return false;
    }

    // 1. Create a new RSA object
    rsa = RSA_new();
    if (rsa == NULL)
    {
        CPRINTS(ctx, "%s:Error creating RSA object: %s\n", __FUNCTION__,
                ERR_error_string(ERR_get_error(), NULL));
        goto err;
    }

    // 2. Convert raw modulus and exponent to BIGNUMs
    n = BN_bin2bn(modulus, modulus_len, NULL);
    if (n == NULL)
    {
        CPRINTS(ctx, "%s:Error converting modulus to BIGNUM: %s\n",
                __FUNCTION__, ERR_error_string(ERR_get_error(), NULL));
        goto err;
    }

    e = BN_new();
    if (e == NULL)
    {
        CPRINTS(ctx, "%s: Error creating BIGNUM for exponent: %s\n",
                __FUNCTION__, ERR_error_string(ERR_get_error(), NULL));
        goto err;
    }
    if (!BN_set_word(e, exponent))
    {
        CPRINTS(ctx, "%s: Error setting exponent word: %s\n", __FUNCTION__,
                ERR_error_string(ERR_get_error(), NULL));
        goto err;
    }

    // Set the public key components. RSA_set0_key takes ownership of n and e.
    if (!RSA_set0_key(rsa, n, e, NULL))
    { // For public key, d is NULL
        CPRINTS(ctx, "%s: Error setting RSA key components: %s\n", __FUNCTION__,
                ERR_error_string(ERR_get_error(), NULL));
        goto err;
    }
    n = NULL; // Clear pointers to prevent double-free
    e = NULL;

    if (RSA_bits(rsa) != expected_modulus_bits)
    {
        CPRINTS(
            ctx,
            "%s: Error: RSA key size (%d bits) does not match expected size for "
            "scheme (%d bits).\n",
            __FUNCTION__, RSA_bits(rsa), expected_modulus_bits);
        goto err;
    }

    // Input validation: Signature length must match modulus length
    if (sig_len != RSA_size(rsa))
    {
        CPRINTS(
            ctx,
            "%s: Error: Signature length (%d) does not match RSA key size (%d).\n",
            __FUNCTION__, sig_len, RSA_size(rsa));
        goto err;
    }

    // 3. Verify the signature
    // RSA_verify handles the decryption, PKCS#1 v1.5 padding check, and hash
    // comparison internally.
    CPRINTS(ctx, "%s: RSA_verify\n", __FUNCTION__);
    CPRINTS(ctx, "%s: hash_nid %d\n", __FUNCTION__, hash_nid);
    CPRINTS(ctx, "%s: digest_len  %d, digest: \n", __FUNCTION__, digest_len);
    for (int i = 0; i < digest_len; i++)
    {
        CPRINTS(ctx, "%x", digest[i]);
    }
    CPRINTS(ctx, "\n");

    CPRINTS(ctx, "%s: sig_len %d, sig: \n", __FUNCTION__, sig_len);
    for (int i = 0; i < sig_len; i++)
    {
        CPRINTS(ctx, "%x", sig[i]);
    }
    CPRINTS(ctx, "\n");

    ret = RSA_verify(hash_nid, digest, digest_len, sig, sig_len, rsa);

    if (ret == 1)
    {
        CPRINTS(ctx, "%s: Signature verification successful!\n", __FUNCTION__);
    }
    else
    {
        CPRINTS(ctx, "%s: Signature verification failed: %s\n", __FUNCTION__,
                ERR_error_string(ERR_get_error(), NULL));
    }

err:
    RSA_free(rsa); // Frees n and e if RSA_set0_key successfully took ownership
    BN_free(n);    // Only if RSA_set0_key failed or was not called
    BN_free(e);    // Only if RSA_set0_key failed or was not called
    return (ret == 1);
    (void)ctx; // make compiler happy when CPRINTS is null statemenet
}

#ifdef __cplusplus
} //  extern "C"
#endif
