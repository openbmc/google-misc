#include "cr51sign_validator.h"

#include "libcr51sign.h"
#include "libcr51sign_support.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <optional>
#include <vector>

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

/* Global buffer to store BIOS file content. */
std::vector<uint8_t> biosFileBuf = {0};

bool prodToDevDowngradeAllowed()
{
    return ALLOW_PROD_TO_DEV_DOWNGRADE;
}

bool isProductionModeTrue()
{
    return IS_PRODUCTION_MODE;
}

int readFromBuf(const void*, uint32_t offset, uint32_t count, uint8_t* buf)
{
    if (count == 0)
        return LIBCR51SIGN_SUCCESS;
    if (!buf)
        return LIBCR51SIGN_ERROR_RUNTIME_FAILURE;
    std::memcpy(buf, biosFileBuf.data() + offset, count);
    return LIBCR51SIGN_SUCCESS;
}

bool readImage(std::string_view fileName, size_t size)
{
    /* Read the whole file into buffer.
     * Compared to implementing POSIX typed 'read' operations, reading SPI flash
     * contents into memory makes it easier to construct input streams needed
     * later. The memory allocated will be freed afterwards. If the SPI size
     * increases in future platforms and makes BMC run-time memory at risk then
     * this can be revisited.
     */
    int fd = open(fileName.data(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return -1;

    biosFileBuf.clear();
    biosFileBuf.reserve(size);

    for (size_t readTotal = 0; readTotal < size;)
    {
        ssize_t ret = read(fd, biosFileBuf.data(), size);
        if (ret <= 0 && errno != EINTR && errno != EAGAIN &&
            errno != EWOULDBLOCK)
        {
            fmt::print(stderr, "Failed to read BIOS file to buffer: ec{}",
                       EXIT_FAILURE);
            close(fd);
            return false;
        }
        readTotal += ret;
    }
    close(fd);

    return true;
}

std::optional<std::vector<uint8_t>> hashDescriptor(struct libcr51sign_ctx* ctx)
{
    // Create the HASH of the CR51 descriptor on the BIOS
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    int ec;

    ec = hash_init(ctx, HASH_SHA2_256);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash init error: ec{}", ec);
        return std::nullopt;
    }

    hash_update(ctx, biosFileBuf.data() + ctx->descriptor.descriptor_offset,
                ctx->descriptor.descriptor_area_size);
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash update error: ec{}", ec);
        return std::nullopt;
    }

    hash_final(ctx, hash.data());
    if (ec)
    {
        fmt::print(stderr, "CR51 Hash final error: ec{}", ec);
        return std::nullopt;
    }

    return hash;
}

std::optional<struct libcr51sign_validated_regions>
    validateDescriptor(struct libcr51sign_ctx* ctx)
{
    struct libcr51sign_intf intf;
    struct libcr51sign_validated_regions image_regions;
    enum libcr51sign_validation_failure_reason ec;

    /* Common functions are from the libcr51sign support header. */
    intf.hash_init = &hash_init;
    intf.hash_update = &hash_update;
    intf.hash_final = &hash_final;
    intf.verify_signature = &verify_signature;
    intf.read_and_hash_update = nullptr;
    intf.read = &readFromBuf;
    intf.prod_to_dev_downgrade_allowed = &prodToDevDowngradeAllowed;
    intf.is_production_mode = &isProductionModeTrue;

    ec = libcr51sign_validate(ctx, &intf, &image_regions);
    if (ec != LIBCR51SIGN_SUCCESS)
    {
        fmt::print(stderr, "CR51 Validate error: {}",
                   libcr51sign_errorcode_to_string(ec));
        return std::nullopt;
    }

    return image_regions;
}

} // namespace cr51
} // namespace google
