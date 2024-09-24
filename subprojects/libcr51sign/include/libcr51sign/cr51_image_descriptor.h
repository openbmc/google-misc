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
 *  - FMAP & HMAP
 *  - BBINFO
 *  - BIOS signature header
 *
 *  Unless explicitly noted all fields are little-endian & offset/size fields
 *  are in bytes. This struct must reside in a IMAGE_REGION_STATIC region.
 *  In the context of Haven it must also reside on a 64K boundary.
 *  The size of the hashed/signed portion of the descriptor region can be
 *  determined solely by parsing the (fixed) image_descriptor struct.
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
 */

// clang-format off
#define IMAGE_REGION_STATIC                     (1 << 0)
#define IMAGE_REGION_COMPRESSED                 (1 << 1)
#define IMAGE_REGION_WRITE_PROTECTED            (1 << 2)
#define IMAGE_REGION_READ_PROTECTED             (1 << 3)
#define IMAGE_REGION_PERSISTENT                 (1 << 4)
#define IMAGE_REGION_PERSISTENT_RELOCATABLE     (1 << 5)
#define IMAGE_REGION_PERSISTENT_EXPANDABLE      (1 << 6)
#define IMAGE_REGION_OVERRIDE                   (1 << 7)
#define IMAGE_REGION_OVERRIDE_ON_TRANSITION     (1 << 8)
#define IMAGE_REGION_MAILBOX                    (1 << 9)
#define IMAGE_REGION_SKIP_BOOT_VALIDATION       (1 << 10)
#define IMAGE_REGION_EMPTY                      (1 << 11)
// clang-format on

/* Little endian on flash. */
#define DESCRIPTOR_MAGIC 0x5f435344474d495f // "_IMGDSC_"
#define HASH_MAGIC 0x48534148               // "HASH"
#define DENYLIST_MAGIC 0x4b434c42           // "BLCK"
#define BLOB_MAGIC 0x424f4c42               // "BLOB"
#define SIGNATURE_MAGIC 0x4e474953          // "SIGN"

/* Values for 'blob_data.blob_type_magic'. Little-endian on flash. */

/* Indicates that 'blob_data.blob_payload' contains a serialized
 * platforms.security.titan.DescriptorExtensions protocol buffer message. There
 * must be zero or one DescriptorExtensions in an image. If more than one is
 * found, the image descriptor is invalid and the image must be treated as
 * unsigned.
 */
#define BLOB_TYPE_MAGIC_DESCRIPTOR_EXTENSIONS 0x58454250 // "PBEX"

/* Indicates that 'blob_data.blob_payload' contains an image_mauv structure.
 * There must be zero or one image_mauv
 * structures in an image. If more than one is found, the image descriptor is
 * invalid and the image must be treated as unsigned.
 */
#define BLOB_TYPE_MAGIC_MAUV 0x5655414D // "MAUV"

/* Indicates that 'blob_data.blob_payload' contains a 32-byte sha256 hash of
 * all the IMAGE_REGION_STATIC partitions that don't have
 * IMAGE_REGION_SKIP_BOOT_VALIDATION set.
 */
#define BLOB_TYPE_MAGIC_BOOT_HASH_SHA256 0x48534842 // "BHSH"

/* Indicates that 'blob_data.blob_payload' contains a lockdown_control
 * structure. There must be zero or
 * one lockdown_status structures in an image. If more than one is found, the
 * image descriptor is invalid and the image must be treated as unsigned.
 */
#define BLOB_TYPE_MAGIC_LOCKDOWN_CONTROL 0x4E444B4C // "LKDN"

/* Indicates the type of the image. The type of the image also indicates the
 * family of key that was used to sign the image. If the image type is signed
 * with a key stored in RKM, then a corresponding enumeration should be added to
 * google3/platforms/security/titan/keyspec.proto.
 *
 * Note: if the image type is IMAGE_UNSIGNED_INTEGRITY, the signature_scheme has
 * to be of type
 * *_NO_SIGNATURE. Also, all other image types cannot transition to an image of
 * type IMAGE_UNSIGNED_INTEGRITY.
 *
 * The way to verify an image of type IMAGE_UNSIGNED_INTEGRITY differs from
 * other types of images as it is not signed with an asymmetric key. Instead,
 * one can verify the integrity by computing the shasum over the descriptor.
 */
enum image_type
{
    IMAGE_DEV = 0,
    IMAGE_PROD = 1,
    IMAGE_BREAKOUT = 2,
    IMAGE_TEST = 3,
    IMAGE_UNSIGNED_INTEGRITY = 4
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

/* Note: If the image is of type IMAGE_UNSIGNED_INTEGRITY, the signature_scheme
 * has to be of type *_ONLY_NO_SIGNATURE.
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

/* Payload image family. Distinct from the Haven image family. */
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
    uint64_t descriptor_magic = 0; // #define DESCRIPTOR_MAGIC
    /* Major revisions of this structure are not backwards compatible. */
    uint8_t descriptor_major = 0;
    /* Minor revisions of this structure are backwards compatible. */
    uint8_t descriptor_minor = 0;
    /* Padding. */
    uint16_t reserved_0 = 0;

    /* This field allows us to mitigate a DOS vector if we end up
     * scanning the image to discover the image descriptor. The offset
     * and size are hashed with the rest of the descriptor to prevent
     * an attacker from copying a valid descriptor to a different
     * location.
     *
     * The offset is relative to the start of the image data.
     */
    uint32_t descriptor_offset = 0;
    /* Includes this struct as well as the auxiliary structs (hash_*,
     * signature_*, denylist, and blob). This many bytes will be skipped when
     * computing the hash of the region this struct resides in. Tail padding is
     * allowed but must be all 0xff's.
     */
    uint32_t descriptor_area_size = 0;

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
    uint32_t image_family = 0;
    /* Follow the Kibbles versioning scheme. */
    uint32_t image_major = 0;
    uint32_t image_minor = 0;
    uint32_t image_point = 0;
    uint32_t image_subpoint = 0;
    /* Seconds since epoch. */
    uint64_t build_timestamp = 0;

    /* image_type enum { DEV, PROD, BREAKOUT, UNSIGNED_INTEGRITY} */
    uint8_t image_type = 0;
    /* 0: no denylist struct, 1: watermark only, >1: watermark + denylist */
    uint8_t denylist_size = 0;
    /* hash_type enum { NONE, SHA2_224, SHA2_256, ...} */
    uint8_t hash_type = 0;
    /* signature_scheme enum { NONE, RSA2048_PKCS15, ...}
     * If set, hash_type must be set as well (cannot be NONE).
     */
    uint8_t signature_scheme = 0;

    /* struct image_region array size. */
    uint8_t region_count = 0;
    uint8_t reserved_1 = 0;
    uint16_t reserved_2 = 0;
    /* The sum of the image_region.region_size fields must add up. */
    uint32_t image_size = 0;
    /* Authenticated opaque data exposed to system software. Must be a multiple
     * of 4 to maintain alignment. Does not include the blob struct magic.
     */
    uint32_t blob_size = 0;
    /* The list is strictly ordered by region_offset.
     * Must exhaustively describe the image.
     */
    struct image_region image_regions[];
} __attribute__((__packed__));

/* Hash the static regions (IMAGE_REGION_STATIC) excluding this descriptor
 * structure i.e. skipping image_descriptor.descriptor_size bytes (optional).
 */
struct hash_sha256
{
    uint32_t hash_magic = 0; // #define HASH_MAGIC
    uint8_t hash[32];
} __attribute__((__packed__));

struct hash_sha512
{
    uint32_t hash_magic = 0; // #define HASH_MAGIC
    uint8_t hash[64];
} __attribute__((__packed__));

struct denylist_record
{
    uint32_t image_major = 0;
    uint32_t image_minor = 0;
    uint32_t image_point = 0;
    uint32_t image_subpoint = 0;
} __attribute__((__packed__));

struct denylist
{
    uint32_t denylist_magic = 0; // #define DENYLIST_MAGIC
    /* Deny list. The first entry is the watermark. All subsequent entries must
     * be newer than the watermark.
     */
    struct denylist_record denylist_record[];
} __attribute__((__packed__));

struct blob
{
    uint32_t blob_magic = 0; // #define BLOB_MAGIC
    /* Array of blob_data structures - see blob_data below for details. */
    uint8_t blobs[];
} __attribute__((__packed__));

/* blob data is expected to be aligned to 4 bytes */
#define BLOB_DATA_ALIGNMENT (4)

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
    uint32_t blob_type_magic = 0;
    /* Size of the data contained in blob_payload. Need not be a multiple of 4
     * bytes. Must have sizeof(struct blob_data) + blob_payload_size <=
     * blob_size.
     */
    uint32_t blob_payload_size = 0;
    uint8_t blob_payload[];
} __attribute__((__packed__));

#define IMAGE_MAUV_STRUCT_VERSION 1

struct image_mauv
{
    /* Version of the MAUV structure. */
    uint32_t mauv_struct_version = 0;

    /* padding for 64-bit alignment of payload_security_version
     * must be set to 0xffffffff */
    uint32_t reserved_0 = 0xffffffff;

    /* The version of the payload in which this `struct image_mauv` was
     * embedded. This would be better inside of `struct image_descriptor`, but
     * that structure doesn't have any spare fields or a reasonable way to grow
     * the structure. When processing firmware updates, the update will be
     * aborted if `payload_security_version` of the update payload is less than
     * the `minimum_acceptable_update_version` in gNVRAM.
     */
    uint64_t payload_security_version = 0;

    /* A monotonic counter that should be increased whenever the
     * `minimum_acceptable_update_version or version_denylist fields are
     * changed. In order for the image_mauv structure in gNVRAM to be updated
     * after an payload update, the `mauv_update_timestamp` field in the new
     * payload must be greater than the `mauv_update_timestamp` field in gNVRAM.
     *
     * Although the firmware doesn't assign any semantic meaning to this value,
     * by convention should be the number of seconds since the unix epoch at the
     * time the payload was signed.
     */
    uint64_t mauv_update_timestamp = 0;

    /* Minimum acceptable update version.  An update to a payload with its
     * `payload_security_version` field less than this field in gNVRAM is
     * forbidden. This value is not monotonic.
     */
    uint64_t minimum_acceptable_update_version = 0;

    /* padding for 64-bit alignment of version_denylist
     * must be set to 0xffffffff */
    uint32_t reserved_1 = 0xffffffff;

    /* Number of entries in the denylist. */
    uint32_t version_denylist_num_entries = 0;

    /* A version denylist.  Updates to any version in this list will be rejected
     * by the firmware.
     */
    uint64_t version_denylist[];
} __attribute__((packed));

_Static_assert(offsetof(struct image_mauv, payload_security_version) %
                       sizeof(uint64_t) ==
                   0,
               "bad payload_security_version alignment");

_Static_assert(
    offsetof(struct image_mauv, version_denylist) % sizeof(uint64_t) == 0,
    "bad denylist alignment");

// When A/B updates are enabled, SPS_EEPROM_LOCKDOWN_IMMUTABLE is invalid.
enum sps_eeprom_lockdown_status
{
    // Unverified or invalid image. All writes allowed.
    SPS_EEPROM_LOCKDOWN_FAILSAFE = 0,
    // Valid image. Static regions are write protected. Write-protected
    // non-static
    // regions are still writable. In single image mode, can
    // transition to SPS_EEPROM_LOCKDOWN_IMMUTABLE from this state.
    SPS_EEPROM_LOCKDOWN_READY = 1,
    // Entire image is write protected outside of the mailbox image region. Note
    // that the payload image may be modified through EC Host mailbox
    // update commands.
    SPS_EEPROM_LOCKDOWN_IMMUTABLE = 2,
    // Valid image. Immutable static and write-protected non-static regions. In
    // single image mode, must reset to update.
    SPS_EEPROM_LOCKDOWN_ENABLED = 3
};

#define LOCKDOWN_CONTROL_STRUCT_VERSION 1

struct lockdown_control
{
    /* Version of the lockdown_status structure. */
    uint32_t lockdown_control_struct_version = 0;

    /* The default lockdown status for a valid signed payload image. The value
     * is identical to `enum sps_eeprom_lockdown_status`. 0 = Failsafe, 1 =
     * Ready.
     */
    uint32_t default_status = 0;
};

/* RSA4096 is the largest key type currently supported. */
#define MAX_KEY_SIZE_NBYTES 512

/* Signature of the hash of the image_descriptor structure up to and including
 * this struct but excluding the signature field (optional).
 */
struct signature_rsa2048_pkcs15
{
    uint32_t signature_magic = 0; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index = 0;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index = 0;
    uint32_t exponent = 0;  // little-endian
    uint8_t modulus[256];   // big-endian
    uint8_t signature[256]; // big-endian
} __attribute__((__packed__));

struct signature_rsa3072_pkcs15
{
    uint32_t signature_magic = 0; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index = 0;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index = 0;
    uint32_t exponent = 0;  // little-endian
    uint8_t modulus[384];   // big-endian
    uint8_t signature[384]; // big-endian
} __attribute__((__packed__));

struct signature_rsa4096_pkcs15
{
    uint32_t signature_magic = 0; // #define SIGNATURE_MAGIC
    /* Monotonic index of the key used to sign the image (starts at 1). */
    uint16_t key_index = 0;
    /* Used to revoke keys, persisted by the enforcer. */
    uint16_t min_key_index = 0;
    uint32_t exponent = 0;  // little-endian
    uint8_t modulus[512];   // big-endian
    uint8_t signature[512]; // big-endian
} __attribute__((__packed__));

struct sha256_only_no_signature
{
    uint32_t signature_magic = 0; // #define SIGNATURE_MAGIC
    uint8_t digest[32];
} __attribute__((__packed__));

#endif // PLATFORMS_SECURITY_TITAN_CR51_IMAGE_DESCRIPTOR_H_
