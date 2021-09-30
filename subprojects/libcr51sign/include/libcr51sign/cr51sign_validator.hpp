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
     * @param[in] imageDescriptor Buffer containing the content of the firmware
     * data
     *
     * @return return the span of the hash
     */
    virtual std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx,
                       std::span<std::byte> imageDescriptor) = 0;

    /** @brief Validate the CR51 Sign Descriptor and return the list of image
     * regions
     *
     * @param[in] ctx         Cr51 signature context
     * @param[in] intf        Cr51 interface
     *
     * For intf,intf->read, intf->retrieve_stored_image_mauv_data, and
     * intf->store_new_image_mauv_data should be set already for the validator
     * to work.
     *
     * @return std::nullopt if failed to validate the descriptor, otherwise
     * return the validated regions
     */
    virtual std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx,
                           struct libcr51sign_intf* intf) = 0;
};

class Cr51SignValidatorIpml : public Cr51SignValidator
{
  public:
    Cr51SignValidatorIpml(bool prodToDev, bool productionMode,
                          uint8_t imageFamily) :
        prodToDev(prodToDev),
        productionMode(productionMode), imageFamily(imageFamily)
    {}

    std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx,
                       std::span<std::byte> imageDescriptor) override;

    std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx,
                           struct libcr51sign_intf* intf) override;

  private:
    bool prodToDev;
    bool productionMode;
    uint8_t imageFamily;
    std::vector<uint8_t> hash;
};

} // namespace cr51
} // namespace google
