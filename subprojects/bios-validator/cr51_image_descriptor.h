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
#ifndef PLATFORMS_SECURITY_TITAN_CR51_IMAGE_DESCRIPTOR_H_
#define PLATFORMS_SECURITY_TITAN_CR51_IMAGE_DESCRIPTOR_H_
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
/* _Static_assert is usually not enabled in C++ mode by compilers. */
#include <assert.h>
#define _Static_assert static_assert
#endif
/* This structure encodes a superset of what we have historically encoded in:
 *
 *  Unless explicitly noted all fields are little-endian & offset/size fields
 *  are in bytes. This struct must reside in a IMAGE_REGION_STATIC region and
 *  must also reside on a 64K boundary. The size of the hashed/signed portion
 *  of the descriptor region can be determined solely by parsing the (fixed)
 *  image_descriptor struct.
 *
 *  --------------------------------Flash layout--------------------------------
 *  |                     struct image_descriptor (signed)                     |
 *  |                struct image_region[region_count] (signed)                |
 *  ----------------------------------------------------------------------------
 *  |               (optional: hash_type) struct hash_* (signed)               |
 *  ----------------------------------------------------------------------------
 *  |           (optional: denylist_size) struct denylist (signed)             |
 *  |             struct denylist_record[denylist_size] (signed)               |
 *  ----------------------------------------------------------------------------
 *  |                (optional: blob_size) struct blob (signed)                |
 *  |                     uint8_t blob[blob_size] (signed)                     |
 *  ----------------------------------------------------------------------------
 *  |    (optional: signature_scheme) struct signature_* (partially signed)    |
 *  ----------------------------------------------------------------------------
 *  |           (optional) struct key_rotation_records (not signed)            |
 *  ----------------------------------------------------------------------------
 */

#define IMAGE_REGION_STATIC (1 << 0)
#define IMAGE_REGION_COMPRESSED (1 << 1)
#define IMAGE_REGION_WRITE_PROTECTED (1 << 2)
#define IMAGE_REGION_READ_PROTECTED (1 << 3)
#define IMAGE_REGION_PERSISTENT (1 << 4)
#define IMAGE_REGION_PERSISTENT_RELOCATABLE (1 << 5)
#define IMAGE_REGION_PERSISTENT_EXPANDABLE (1 << 6)

/* Little endian on flash. */
#define DESCRIPTOR_MAGIC 0x5f435344474d495f // "_IMGDSC_"
#define HASH_MAGIC 0x48534148               // "HASH"
#define DENYLIST_MAGIC 0x4b434c42           // "BLCK"
#define BLOB_MAGIC 0x424f4c42               // "BLOB"
#define SIGNATURE_MAGIC 0x4e474953          // "SIGN"
#define ROTATION_MAGIC 0x5254524b           // "KRTR"

/* Indicates the type of the image. The type of the image also indicates the
 * family of key that was used to sign the image.
 *
 * Note: if the image type is IMAGE_SELF, the signature_scheme has to be of type
 * *_NO_SIGNATURE. Also, all other image types cannot transition to an image of
 * type IMAGE_SELF.
 *
 * The way to verify an image of type IMAGE_SELF differs from
 * other types of images as it is not signed with an asymmetric key. Instead,
 * one can verify the integrity by computing the shasum over the descriptor.
 */
enum image_type
{
    IMAGE_DEV = 0,
    IMAGE_PROD = 1,
    IMAGE_BREAKOUT = 2,
    IMAGE_TEST = 3,
    IMAGE_SELF = 4
};

enum hash_type
{
    HASH_NONE = 0,
    HASH_SHA2_224 = 1,
    HASH_SHA2_256 = 2,
    HASH_SHA2_384 = 3,
    HASH_SHA2_512 = 4,
    HASH_SHA3_224 = 5,
    HASH_SHA3_256 = 6,
    HASH_SHA3_384 = 7,
    HASH_SHA3_512 = 8
};

/* Note: If the image is of type IMAGE_SELF, the signature_scheme has to be of
 * type *_ONLY_NO_SIGNATURE.
 */
enum signature_scheme
{
    SIGNATURE_NONE = 0,
    SIGNATURE_RSA2048_PKCS15 = 1,
    SIGNATURE_RSA3072_PKCS15 = 2,
    SIGNATURE_RSA4096_PKCS15 = 3,
    SIGNATURE_RSA4096_PKCS15_SHA512 = 4,
    SHA256_ONLY_NO_SIGNATURE = 5
};

/* Payload image family. */
enum image_family
{
    IMAGE_FAMILY_ALL = 0,
    //  values < 256 are reserved for Google-internal use
};

#define IMAGE_REGION_PROTECTED_ALIGNMENT (4096)
#define IMAGE_REGION_PROTECTED_PAGE_LENGTH (4096)

struct image_region
{
    uint8_t region_name[32]; // null-terminated ASCII string
    uint32_t region_offset;  // read- and write- protected regions must be
                             // aligned to IMAGE_REGION_PROTECTED_ALIGNMENT.
                             // Other regions are also aligned which
                             // simplifies their implementation.
    uint32_t region_size;    // read- and write- protected regions must be a
                             // multiple of IMAGE_REGION_PROTECTED_PAGE_LENGTH.
    /* Regions will not be persisted across different versions.
     * This field is intended to flag potential incompatibilities in the
     * context of data migration (e.g. the ELOG format changed between
     * two BIOS releases).
     */
    uint16_t region_version;
    /* See IMAGE_REGION_* defines above. */
    uint16_t region_attributes;
} __attribute__((__packed__));

/* Main structure (major=1, minor=0). Verification process:
 * - Hash(image_descriptor + region_count * struct image_region +
 *        struct hash +
 *        struct denylist + denylist_size * struct denylist_record +
 *        struct blob + uint8_t blob[blob_size])
 * - Verify the signature_* over the hash computed in the previous step
 * - Compute the rolling hash of the regions marked IMAGE_REGION_STATIC
 * - The image descriptor is excluded from the hash (descriptor_size bytes)
 * - Compare the computed hash to the struct hash_*.hash
 */
struct image_descriptor
{
    uint64_t descriptor_magic; // #define DESCRIPTOR_MAGIC
    /* Major revisions of this structure are not backwards compatible. */
    uint8_t descriptor_major;
    /* Minor revisions of this structure are backwards compatible. */
    uint8_t descriptor_minor;
    /* Padding. */
    uint16_t reserved_0;

    /* This field allows us to mitigate a DOS vector if we end up
     * scanning the image to discover the image descriptor. The offset
     * and size are hashed with the rest of the descriptor to prevent
     * an attacker from copying a valid descriptor to a different
     * location.
     *
     * The offset is relative to the start of the image data.
     */
    uint32_t descriptor_offset;
    /* Includes this struct as well as the auxiliary structs (hash_*,
     * signature_*, denylist, blob & key_rotation_records). This many bytes
     * will be skipped when computing the hash of the region this struct
     * resides in. Tail padding is allowed but must be all 0xff's.
     */
    uint32_t descriptor_area_size;

    /*** Image information. ***/

    /* Null-terminated ASCII string. For BIOS this would be the platform
     * family-genus-version-date (e.g. ixion-hsw-2.8.0-2017.10.03).
     * Intended for consumption by system software that generates human
     * readable output (e.g. gsys).
     */
    uint8_t image_name[32];
    /* Image transitions are enforced to be from/to the same family.
     * 0 is treated as a wildcard (can upgrade to/from any image family).
     * See image_family enum above.
     */
    uint32_t image_family;
    uint32_t image_major;
    uint32_t image_minor;
    uint32_t image_point;
    uint32_t image_subpoint;
    /* Seconds since epoch. */
    uint64_t build_timestamp;

    /* image_type enum { DEV, PROD, BREAKOUT, SELF} */
    uint8_t image_type;
    /* 0: no denylist struct, 1: watermark only, >1: watermark + denylist */
    uint8_t denylist_size;
    /* hash_type enum { NONE, SHA2_224, SHA2_256, ...} */
    uint8_t hash_type;
    /* signature_scheme enum { NONE, RSA2048_PKCS15, ...}
     * If set, hash_type must be set as well (cannot be NONE).
     */
    uint8_t signature_scheme;

    /* struct image_region array size. */
    uint8_t region_count;
    uint8_t reserved_1;
    uint16_t reserved_2;
    /* The sum of the image_region.region_size fields must add up. */
    uint32_t image_size;
    /* Authenticated opaque data exposed to system software. Must be a multiple
     * of 4 to maintain alignment. Does not include the blob struct magic.
     */
    uint32_t blob_size;
    /* The list is strictly ordered by region_offset.
     * Must exhaustively describe the image.
     */
#ifndef OMIT_VARIABLE_ARRAYS
    struct image_region image_regions[];
#endif
} __attribute__((__packed__));

/* Hash the static regions (IMAGE_REGION_STATIC) excluding this descriptor
 * structure i.e. skipping image_descriptor.descriptor_size bytes (optional).
 */
struct hash_sha256
{
    uint32_t hash_magic; // #define HASH_MAGIC
    uint8_t hash[32];
} __attribute__((__packed__));

struct hash_sha512
{
    uint32_t hash_magic; // #define HASH_MAGIC
    uint8_t hash[64];
} __attribute__((__packed__));

struct denylist_record
{
    uint32_t image_major;
    uint32_t image_minor;
    uint32_t image_point;
    uint32_t image_subpoint;
} __attribute__((__packed__));

struct denylist
{
    uint32_t denylist_magic; // #define DENYLIST_MAGIC
    /* Deny list. The first entry is the watermark. All subsequent entries must
     * be newer than the watermark.
     */
#ifndef OMIT_VARIABLE_ARRAYS
    struct denylist_record denylist_record[];
#endif
} __attribute__((__packed__));

struct blob
{
    uint32_t blob_magic; // #define BLOB_MAGIC
#ifndef OMIT_VARIABLE_ARRAYS
    /* Array of blob_data structures - see blob_data below for details. */
    uint8_t blobs[];
#endif
} __attribute__((__packed__));

/* If blobs[] is non-empty, it is expected to contain one more more instances
 * of this struct. Each blob_data is followed by the minimum number of padding
 * bytes (0-3) needed to maintain 4-byte alignment of blob_data structures.
 * Padding bytes must be 0xFF and must be ignored by readers of blobs[].
 *
 * The ordering of the blob_data structures is undefined. Readers of blobs[]
 * must locate expected blob_data by inspecting blob_type_magic of each
 * blob_data. Readers are expected to ignore unknown blob_type_magic values,
 * skipping over them to allow for future types.
 *
 * If blob_size is greater than zero but less than sizeof(struct blob_data), the
 * blobs list is invalid. The blobs list is also invalid if there are multiple
 * blob_data structures and the last one is truncated due to blob_size being too
 * small to hold blob_payload_size. Readers must walk the entire length of the
 * blob_data list to validate the list is well-formed. Any image with an
 * invalid blobs list has an invalid descriptor and must be treated the same as
 * an unsigned image.
 */
struct blob_data
{
    /* BLOB_TYPE_MAGIC_* */
    uint32_t blob_type_magic;
    /* Size of the data contained in blob_payload. Need not be a multiple of 4
     * bytes. Must have sizeof(struct blob_data) + blob_payload_size <=
     * blob_size.
     */
    uint32_t blob_payload_size;
#ifndef OMIT_VARIABLE_ARRAYS
    uint8_t blob_payload[];
#endif
} __attribute__((__packed__));

/* Signature of the hash of the image_descriptor structure up to and including
 * this struct but excluding the signature field (optional).
 */
struct signature_rsa2048_pkcs15
{
    uint32_t signature_magic; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index;
    uint32_t exponent;      // little-endian
    uint8_t modulus[256];   // big-endian
    uint8_t signature[256]; // big-endian
} __attribute__((__packed__));

struct signature_rsa3072_pkcs15
{
    uint32_t signature_magic; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index;
    uint32_t exponent;      // little-endian
    uint8_t modulus[384];   // big-endian
    uint8_t signature[384]; // big-endian
} __attribute__((__packed__));

struct signature_rsa4096_pkcs15
{
    uint32_t signature_magic; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index;
    uint32_t exponent;      // little-endian
    uint8_t modulus[512];   // big-endian
    uint8_t signature[512]; // big-endian
} __attribute__((__packed__));

struct sha256_only_no_signature
{
    uint32_t signature_magic; // #define SIGNATURE_MAGIC
    uint8_t digest[32];
} __attribute__((__packed__));

/* Key rotation record (optional).
 * Enables enforcers to verify images signed with newer (rotated) keys.
 * The hash function, signature & padding schemes are currently pinned
 * by image family. This struct is likely to evolve.
 */
struct record_rsa2048_pkcs15
{
    uint16_t from_index;
    uint16_t to_index;
    uint32_t exponent;    // exponent of the new key, little-endian
    uint8_t modulus[256]; // modulus of the new key, big-endian
    /* SIGN[K<from_index>](HASH(to_index (LE) | exponent (LE) | modulus (BE)))
     */
    uint8_t signature[256]; // big-endian
} __attribute__((__packed__));

struct record_rsa3072_pkcs15
{
    uint16_t from_index;
    uint16_t to_index;
    uint32_t exponent;    // exponent of the new key, little-endian
    uint8_t modulus[384]; // modulus of the new key, big-endian
    /* SIGN[K<from_index>](HASH(to_index (LE) | exponent (LE) | modulus (BE)))
     */
    uint8_t signature[384]; // big-endian
} __attribute__((__packed__));

struct record_rsa4096_pkcs15
{
    uint16_t from_index;
    uint16_t to_index;
    uint32_t exponent;    // exponent of the new key, little-endian
    uint8_t modulus[512]; // modulus of the new key, big-endian
    /* SIGN[K<from_index>](HASH(to_index (LE) | exponent (LE) | modulus (BE)))
     */
    uint8_t signature[512]; // big-endian
} __attribute__((__packed__));

struct key_rotation_records_rsa2048_pkcs15
{
    uint32_t rotation_magic; // #define ROTATION_MAGIC
    uint16_t record_count;
    uint16_t reserved_0;
#ifndef OMIT_VARIABLE_ARRAYS
    struct record_rsa2048_pkcs15 records[];
#endif
} __attribute__((__packed__));

struct key_rotation_records_rsa3072_pkcs15
{
    uint32_t rotation_magic; // #define ROTATION_MAGIC
    uint16_t record_count;
    uint16_t reserved_0;
#ifndef OMIT_VARIABLE_ARRAYS
    struct record_rsa3072_pkcs15 records[];
#endif
} __attribute__((__packed__));

struct key_rotation_records_rsa4096_pkcs15
{
    uint32_t rotation_magic; // #define ROTATION_MAGIC
    uint16_t record_count;
    uint16_t reserved_0;
#ifndef OMIT_VARIABLE_ARRAYS
    struct record_rsa3072_pkcs15 records[];
#endif
} __attribute__((__packed__));
#endif // PLATFORMS_SECURITY_TITAN_CR51_IMAGE_DESCRIPTOR_H_
