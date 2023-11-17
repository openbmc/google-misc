#include "stddef.h"

#include <libcr51sign/cr51_image_descriptor.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_internal.h>
#include <libcr51sign/libcr51sign_mauv.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IMAGE_MAUV_MAX_DENYLIST_ENTRIES                                        \
    ((IMAGE_MAUV_DATA_MAX_SIZE - sizeof(struct image_mauv)) / sizeof(uint64_t))

_Static_assert(
    (sizeof(struct image_mauv) +
     IMAGE_MAUV_MAX_DENYLIST_ENTRIES *
         MEMBER_SIZE(struct image_mauv, version_denylist[0])) ==
        IMAGE_MAUV_DATA_MAX_SIZE,
    "IMAGE_MAUV_MAX_DENYLIST_ENTRIES number of denylist entries do not "
    "completely fill IMAGE_MAUV_MAX_SIZE bytes assumed for data in struct "
    "image_mauv");

// LINT.IfChange(full_mauv_def)
// Use wrapper struct to preserve alignment of image_mauv
struct full_mauv
{
    struct image_mauv mauv;
    uint8_t extra[IMAGE_MAUV_DATA_MAX_SIZE - sizeof(struct image_mauv)];
};
// LINT.ThenChange()

// Verify BLOB magic bytes in payload's image descriptor at the expected offset
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] payload_blob_offset  Absolute offset of payload BLOB data in
//                                 payload's image descriptor
//
// @return `failure_reason`
static failure_reason
    verify_payload_blob_magic(const struct libcr51sign_ctx* const ctx,
                              const struct libcr51sign_intf* const intf,
                              const uint32_t payload_blob_offset)
{
    int irv = 0;
    struct blob payload_blob = {0};

    if (!intf->read)
    {
        CPRINTS(ctx, "%s: Missing interface intf->read\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }

    irv = intf->read(ctx, payload_blob_offset, sizeof(struct blob),
                     (uint8_t*)&payload_blob);
    if (irv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: Could not read BLOB magic bytes from payload\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    if (payload_blob.blob_magic != BLOB_MAGIC)
    {
        CPRINTS(ctx, "%s: BLOB magic bytes read from payload(%x) are invalid\n",
                __FUNCTION__, payload_blob.blob_magic);
        return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
    }

    return LIBCR51SIGN_SUCCESS;
}

// Find offset of Image MAUV data in payload BLOB inside the image descriptor
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] offset_after_payload_blob_magic  Absolute offset of data after
//                                             payload BLOB magic bytes in image
//                                             descriptor
// @param[in] payload_blob_size  Size of payload BLOB as per its image
//                               descriptor
// @param[out] payload_image_mauv_data_offset  Absolute offset of Image MAUV
//                                             data in payload's image
//                                             descriptor
// @param[out] payload_image_mauv_data_size  Size of Image MAUV data embedded in
//                                           payload's image descriptor
//
// @return `failure_reason`
static failure_reason find_image_mauv_data_offset_in_payload(
    const struct libcr51sign_ctx* const ctx,
    const struct libcr51sign_intf* const intf,
    const uint32_t offset_after_payload_blob_magic,
    const uint32_t payload_blob_size,
    uint32_t* const restrict payload_image_mauv_data_offset,
    uint32_t* const restrict payload_image_mauv_data_size)
{
    struct blob_data payload_blob_data = {0};
    int irv = 0;
    bool found_image_mauv_data = false;

    if (!intf->read)
    {
        CPRINTS(ctx, "%s: Missing interface intf->read\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }
    for (uint32_t current_offset = offset_after_payload_blob_magic;
         current_offset <= offset_after_payload_blob_magic + payload_blob_size -
                               sizeof(struct blob_data);
         /* increment based on each blob_data's size in loop */)
    {
        irv = intf->read(ctx, current_offset, sizeof(struct blob_data),
                         (uint8_t*)&payload_blob_data);
        if (irv != LIBCR51SIGN_SUCCESS)
        {
            CPRINTS(ctx, "%s: Could not read BLOB data at offset %x\n",
                    __FUNCTION__, current_offset);
            return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
        }

        if ((current_offset - offset_after_payload_blob_magic) +
                sizeof(struct blob_data) + payload_blob_data.blob_payload_size >
            payload_blob_size)
        {
            CPRINTS(
                ctx,
                "%s: BLOB payload size crosses threshold expected by blob_size "
                "in image descriptor",
                __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
        }

        switch (payload_blob_data.blob_type_magic)
        {
            case BLOB_TYPE_MAGIC_MAUV:
                if (!found_image_mauv_data)
                {
                    *payload_image_mauv_data_offset = current_offset +
                                                      sizeof(struct blob_data);
                    *payload_image_mauv_data_size =
                        payload_blob_data.blob_payload_size;
                    found_image_mauv_data = true;
                    /* intentional fall-through to increment current offset */
                }
                else
                {
                    /* There should be only one Image MAUV in a valid image
                     * descriptor */
                    CPRINTS(
                        ctx,
                        "%s: Found multiple Image MAUV BLOB instances in payload\n",
                        __FUNCTION__);
                    return LIBCR51SIGN_ERROR_INVALID_DESCRIPTOR;
                }
            default:
                current_offset += sizeof(struct blob_data) +
                                  payload_blob_data.blob_payload_size;
                /* Increment offset to keep the expected alignment */
                current_offset =
                    ((current_offset - 1) & ~(BLOB_DATA_ALIGNMENT - 1)) +
                    BLOB_DATA_ALIGNMENT;
                break;
        }
    }
    if (!found_image_mauv_data)
    {
        CPRINTS(ctx, "%s: Did not find Image MAUV BLOB inside payload\n",
                __FUNCTION__);
    }
    return LIBCR51SIGN_SUCCESS;
}

// Read Image MAUV data from BLOB inside payload's image descriptor
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] payload_image_mauv_data_offset  Absolute offset of Image MAUV data
//                                            in payload's image descriptor
// @param[in] payload_image_mauv_data_size  Size of Image MAUV data embedded in
//                                          payload's image descriptor
// @param[out] payload_image_mauv_data_buffer  Buffer to store Image MAUV data
//                                             read from payload's image
//                                             descriptor
//
// @return `failure_reason`
static failure_reason read_image_mauv_data_from_payload(
    const struct libcr51sign_ctx* const ctx,
    const struct libcr51sign_intf* const intf,
    const uint32_t payload_image_mauv_data_offset,
    const uint32_t payload_image_mauv_data_size,
    struct image_mauv* const restrict payload_image_mauv_data_buffer)
{
    int irv = 0;

    if (!intf->read)
    {
        CPRINTS(ctx, "%s: Missing interface intf->read\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }

    if (payload_image_mauv_data_size > IMAGE_MAUV_DATA_MAX_SIZE)
    {
        CPRINTS(
            ctx,
            "%s: Payload Image MAUV data size (0x%x) is more than supported "
            "maximum size\n",
            __FUNCTION__, payload_image_mauv_data_size);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    irv = intf->read(ctx, payload_image_mauv_data_offset,
                     payload_image_mauv_data_size,
                     (uint8_t*)payload_image_mauv_data_buffer);
    if (irv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx,
                "%s: Could not read Image MAUV data from payload @ offset 0x%x "
                "size 0x%x\n",
                __FUNCTION__, payload_image_mauv_data_offset,
                payload_image_mauv_data_size);
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    }

    return LIBCR51SIGN_SUCCESS;
}

// Check if Image MAUV allows update to a target payload version
//
// @param[in] stored_image_mauv_data  Image MAUV data stored in system
// @param[in] new_payload_security_version  Payload version
//
// @return `bool`  `True` if update to target version is allowed by MAUV data
// LINT.IfChange(mauv_update_allowed_checks)
static bool does_stored_image_mauv_allow_update(
    const struct image_mauv* const stored_image_mauv_data,
    const uint64_t new_payload_security_version)
{
    if (new_payload_security_version <
        stored_image_mauv_data->minimum_acceptable_update_version)
    {
        return false;
    }

    for (uint32_t i = 0;
         i < stored_image_mauv_data->version_denylist_num_entries; i++)
    {
        if (stored_image_mauv_data->version_denylist[i] ==
            new_payload_security_version)
        {
            return false;
        }
    }

    return true;
}
// LINT.ThenChange()

// Do a sanity check for values stored in Image MAUV data
//
// @param[in] image_mauv_data  Image MAUV data
// @param[in] image_mauv_data_size  Size of Image MAUV data in bytes
//
// @return `failure_reason`
// LINT.IfChange(sanity_check_mauv)
static failure_reason sanity_check_image_mauv_data(
    const struct image_mauv* const restrict image_mauv_data,
    const uint32_t image_mauv_data_size)
{
    uint32_t expected_image_mauv_data_size = 0;

    if (image_mauv_data_size < sizeof(struct image_mauv))
    {
        CPRINTS(ctx, "%s: Image MAUV data size is smaller than expected\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    if (image_mauv_data->mauv_struct_version != IMAGE_MAUV_STRUCT_VERSION)
    {
        CPRINTS(ctx, "%s: Unexpected Image MAUV version\n", __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    if (image_mauv_data->payload_security_version == 0)
    {
        // Handle trivial case of someone not initializing MAUV properly
        CPRINTS(ctx, "%s: Payload security version should be greater than 0\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    if (image_mauv_data->version_denylist_num_entries >
        IMAGE_MAUV_MAX_DENYLIST_ENTRIES)
    {
        CPRINTS(
            ctx,
            "%s: Version denylist entries in Image MAUV exceed maximum count\n",
            __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    expected_image_mauv_data_size =
        sizeof(struct image_mauv) +
        image_mauv_data->version_denylist_num_entries *
            MEMBER_SIZE(struct image_mauv, version_denylist[0]);

    if (image_mauv_data_size != expected_image_mauv_data_size)
    {
        CPRINTS(ctx,
                "%s: Size of Image MAUV data (0x%x) is different than expected "
                "size (0x%x)\n",
                __FUNCTION__, image_mauv_data_size,
                expected_image_mauv_data_size);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    if (!does_stored_image_mauv_allow_update(
            image_mauv_data, image_mauv_data->payload_security_version))
    {
        CPRINTS(ctx,
                "%s: Image MAUV does not allow update to the payload it was "
                "contained in\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }
    return LIBCR51SIGN_SUCCESS;
}
// LINT.ThenChange()

// Find and read (if found) Image MAUV from payload's image descriptor
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] payload_blob_offset  Absolute offset of payload BLOB data in
//                                 payload's image descriptor
// @param[in] payload_blob_size  Size of payload BLOB data as per payload's
//                               image descriptor
// @param[out] payload_image_mauv_data_buffer  Buffer to store Image MAUV data
//                                             read from payload's image
//                                             descriptor
// @param[out] payload_image_mauv_data_size  Size of Image MAUV data (in bytes)
//                                           read from payload's image
//                                           descriptor
// @param[out] payload_contains_image_mauv_data  Flag to indicate whether Image
//                                               MAUV data is present in
//                                               payload's image descriptor
//
// @return `failure_reason`
failure_reason find_and_read_image_mauv_data_from_payload(
    const struct libcr51sign_ctx* const ctx,
    const struct libcr51sign_intf* const intf,
    const uint32_t payload_blob_offset, const uint32_t payload_blob_size,
    uint8_t payload_image_mauv_data_buffer[],
    uint32_t* const restrict payload_image_mauv_data_size,
    bool* const restrict payload_contains_image_mauv_data)
{
    failure_reason rv = LIBCR51SIGN_SUCCESS;
    uint32_t payload_image_mauv_data_offset = 0;

    rv = verify_payload_blob_magic(ctx, intf, payload_blob_offset);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    rv = find_image_mauv_data_offset_in_payload(
        ctx, intf, payload_blob_offset + offsetof(struct blob, blobs),
        payload_blob_size, &payload_image_mauv_data_offset,
        payload_image_mauv_data_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    *payload_contains_image_mauv_data = (payload_image_mauv_data_offset != 0);

    if (*payload_contains_image_mauv_data)
    {
        rv = read_image_mauv_data_from_payload(
            ctx, intf, payload_image_mauv_data_offset,
            *payload_image_mauv_data_size,
            (struct image_mauv*)payload_image_mauv_data_buffer);
        if (rv != LIBCR51SIGN_SUCCESS)
        {
            return rv;
        }

        return sanity_check_image_mauv_data(
            (struct image_mauv*)payload_image_mauv_data_buffer,
            *payload_image_mauv_data_size);
    }
    return LIBCR51SIGN_SUCCESS;
}

// Replace stored Image MAUV data with payload Image MAUV data
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] payload_image_mauv_data  Image MAUV data from payload
// @param[in] payload_image_mauv_data_size  Size of Image MAUV data (in bytes)
//                                          embedded inside payload
//
// @return `failure_reason`
static failure_reason update_stored_image_mauv_data(
    const struct libcr51sign_ctx* const ctx,
    const struct libcr51sign_intf* const intf,
    const struct image_mauv* const restrict payload_image_mauv_data,
    const uint32_t payload_image_mauv_data_size)
{
    int irv = 0;

    if (!intf->store_new_image_mauv_data)
    {
        CPRINTS(ctx, "%s: Missing interface intf->store_new_image_mauv_data\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
    }

    irv = intf->store_new_image_mauv_data(
        ctx, (uint8_t*)payload_image_mauv_data, payload_image_mauv_data_size);
    if (irv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx,
                "%s: Could not store new Image MAUV data from the payload\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_STORING_NEW_IMAGE_MAUV_DATA;
    }
    return LIBCR51SIGN_SUCCESS;
}

// Validates Image MAUV from payload against stored Image MAUV (if present)
//
// @param[in] ctx  context which describes the image and holds opaque private
//                 data for the user of the library
// @param[in] intf  function pointers which interface to the current system
//                  and environment
// @param[in] payload_blob_offset  Absolute offset of BLOB data embedded in
//                                 image descriptor. `0` if BLOB data is not
//                                 present in image descriptor
// @param[in] payload_blob_size  Size of BLOB data from `blob_size` field in
//                               image descriptor
//
// @return `failure_reason`
failure_reason
    validate_payload_image_mauv(const struct libcr51sign_ctx* const ctx,
                                const struct libcr51sign_intf* const intf,
                                const uint32_t payload_blob_offset,
                                const uint32_t payload_blob_size)
{
    uint32_t payload_image_mauv_data_size = 0;
    struct full_mauv payload_image_mauv_data_buffer = {0};

    uint32_t stored_image_mauv_data_size = 0;
    struct full_mauv stored_image_mauv_data_buffer = {0};

    bool payload_contains_image_mauv_data = false;

    failure_reason rv = LIBCR51SIGN_SUCCESS;
    int irv = 0;

    bool payload_blob_present = (payload_blob_offset != 0);
    if (payload_blob_present)
    {
        rv = find_and_read_image_mauv_data_from_payload(
            ctx, intf, payload_blob_offset, payload_blob_size,
            (uint8_t*)&payload_image_mauv_data_buffer,
            &payload_image_mauv_data_size, &payload_contains_image_mauv_data);
        if (rv != LIBCR51SIGN_SUCCESS)
        {
            return rv;
        }
    }

    if (!intf->retrieve_stored_image_mauv_data)
    {
        if (payload_contains_image_mauv_data)
        {
            CPRINTS(
                ctx,
                "%s: Payload contains Image MAUV data but required interface "
                "intf->retrieve_stored_image_mauv_data is missing\n",
                __FUNCTION__);
            return LIBCR51SIGN_ERROR_INVALID_INTERFACE;
        }
        CPRINTS(
            ctx,
            "%s: Payload does not contain Image MAUV data and interface "
            "intf->retrieve_stored_image_mauv_data is missing. Skipping MAUV "
            "check for backward compatibility.\n",
            __FUNCTION__);
        return LIBCR51SIGN_SUCCESS;
    }

    irv = intf->retrieve_stored_image_mauv_data(
        ctx, (uint8_t*)&stored_image_mauv_data_buffer,
        &stored_image_mauv_data_size, IMAGE_MAUV_DATA_MAX_SIZE);
    if (irv == LIBCR51SIGN_NO_STORED_MAUV_FOUND)
    {
        CPRINTS(
            ctx,
            "%s: Stored Image MAUV data not present in the system. Skipping Image "
            "MAUV check\n",
            __FUNCTION__);
        if (payload_contains_image_mauv_data)
        {
            return update_stored_image_mauv_data(
                ctx, intf, (struct image_mauv*)&payload_image_mauv_data_buffer,
                payload_image_mauv_data_size);
        }
        return LIBCR51SIGN_SUCCESS;
    }
    if (irv != LIBCR51SIGN_SUCCESS)
    {
        CPRINTS(ctx, "%s: Could not retrieve Image MAUV stored in system\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_RETRIEVING_STORED_IMAGE_MAUV_DATA;
    }
    if (stored_image_mauv_data_size > IMAGE_MAUV_DATA_MAX_SIZE)
    {
        CPRINTS(ctx,
                "%s: Stored Image MAUV data size (0x%x) is more than supported "
                "maximum size\n",
                __FUNCTION__, stored_image_mauv_data_size);
        return LIBCR51SIGN_ERROR_INVALID_IMAGE_MAUV_DATA;
    }

    rv = sanity_check_image_mauv_data(
        (struct image_mauv*)&stored_image_mauv_data_buffer,
        stored_image_mauv_data_size);
    if (rv != LIBCR51SIGN_SUCCESS)
    {
        return rv;
    }

    if (!payload_contains_image_mauv_data)
    {
        CPRINTS(ctx, "%s: Image MAUV expected to be present in payload",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_EXPECTS_PAYLOAD_IMAGE_MAUV;
    }

    if (!does_stored_image_mauv_allow_update(
            (struct image_mauv*)&stored_image_mauv_data_buffer,
            ((struct image_mauv*)&payload_image_mauv_data_buffer)
                ->payload_security_version))
    {
        CPRINTS(ctx,
                "%s: Stored Image MAUV data does not allow update to payload "
                "version\n",
                __FUNCTION__);
        return LIBCR51SIGN_ERROR_STORED_IMAGE_MAUV_DOES_NOT_ALLOW_UPDATE_TO_PAYLOAD;
    }

    if (((struct image_mauv*)&payload_image_mauv_data_buffer)
            ->mauv_update_timestamp >
        ((struct image_mauv*)&stored_image_mauv_data_buffer)
            ->mauv_update_timestamp)
    {
        return update_stored_image_mauv_data(
            ctx, intf, (struct image_mauv*)&payload_image_mauv_data_buffer,
            payload_image_mauv_data_size);
    }
    return LIBCR51SIGN_SUCCESS;
}

#ifdef __cplusplus
} //  extern "C"
#endif
