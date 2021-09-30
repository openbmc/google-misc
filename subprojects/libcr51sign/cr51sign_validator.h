#pragma once
#include "libcr51sign.h"

#include <optional>
#include <string_view>
#include <vector>

namespace google
{
namespace cr51
{

std::vector<uint8_t>& getBuffer();

bool readImage(std::string_view fileName, size_t size);

std::optional<std::vector<uint8_t>> hashDescriptor(struct libcr51sign_ctx* ctx);

std::optional<struct libcr51sign_validated_regions>
    validateDescriptor(struct libcr51sign_ctx* ctx);

} // namespace cr51
} // namespace google