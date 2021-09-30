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
#include "libcr51sign.h"

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

    /** @brief Return the buffer used to store the image.
     *
     * @return the span of the firmware data
     */
    virtual std::span<const uint8_t> getBuffer() = 0;

    /** @brief Read the image and populate the buffer
     *
     * @param[in] fileName  Image file name
     * @param[in] size      Image size
     *
     * @return true if the image is read successfully, otherwise return false
     */
    virtual bool readImage(const char* fileName, size_t size) = 0;

    /** @brief Create the hash of the static regions of the image
     *
     * @param[in] ctx    Cr51 signature context
     *
     * @return return the span of the hash
     */
    virtual std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx) = 0;

    /** @brief Validate the CR51 Sign Descriptor and return the list of image
     * regions
     *
     * @param[in] ctx    Cr51 signature context
     *
     * @return std::nullopt if failed to validate the descriptor, otherwise
     * return the validated regions
     */
    virtual std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx) = 0;
};

class Cr51SignValidatorIpml : public Cr51SignValidator
{
  public:
    std::span<const uint8_t> getBuffer() override;

    bool readImage(const char* fileName, size_t size) override;

    std::span<const uint8_t>
        hashDescriptor(struct libcr51sign_ctx* ctx) override;

    std::optional<struct libcr51sign_validated_regions>
        validateDescriptor(struct libcr51sign_ctx* ctx) override;

  private:
    std::vector<uint8_t> firmwareFileBuf;
    std::vector<uint8_t> hash;
};

} // namespace cr51
} // namespace google
