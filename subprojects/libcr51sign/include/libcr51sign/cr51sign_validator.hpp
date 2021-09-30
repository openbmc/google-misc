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

#pragma once
#include <libcr51sign/libcr51sign.h>

#include <optional>
#include <span>
#include <vector>

namespace google
{
namespace cr51
{

/** @class Cr51SignValidator
 *  @brief Validate the firmware with CR51 signature.
 */
class Cr51SignValidator
{
  public:
    virtual ~Cr51SignValidator() = default;

    /** @brief Create the hash of the static regions of the image
     *
     * @param[in] ctx         Cr51 signature context
     * @param[in] firmwareBuf Buffer containing the content of the firmware data
     *
     * @return return the span of the hash
     */
    virtual std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx,
                       std::span<const uint8_t> firmwareBuf) = 0;

    /** @brief Validate the CR51 Sign Descriptor and return the list of image
     * regions
     *
     * @param[in] ctx         Cr51 signature context
     * @param[in] firmwareBuf Buffer containing the content of the firmware
     * data
     *
     * @return std::nullopt if failed to validate the descriptor, otherwise
     * return the validated regions
     */
    virtual std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx,
                           std::span<const uint8_t> firmwareBuf) = 0;
};

class Cr51SignValidatorIpml : public Cr51SignValidator
{
  public:
    std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx,
                       std::span<const uint8_t> firmwareBuf) override;

    std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx,
                           std::span<const uint8_t> firmwareBuf) override;

  private:
    std::vector<uint8_t> hash;
};

} // namespace cr51
} // namespace google
