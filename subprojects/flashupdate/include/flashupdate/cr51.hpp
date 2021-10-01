// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <libcr51sign/cr51_image_descriptor.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_support.h>

#include <flashupdate/config.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace flashupdate
{
namespace cr51
{

/** @class Cr51
 *  @brief Wrapper for CR51 related methods and libcr51sign.
 */
class Cr51
{
  public:
    Cr51() : hash(std::vector<uint8_t>(SHA256_DIGEST_LENGTH)){};
    virtual ~Cr51() = default;

    /** @brief Validate CR51 image of a image
     *
     * @param[in] file   BIOS image to be validated for CR51 signature
     * @param[in] size   BIOS image size
     * @param[in] keys   BIOS public keys (prod and dev)
     *
     * @return true if the image is properly validated
     */
    virtual bool validateImage(std::string_view file, uint32_t size,
                               Config::Key keys) = 0;

    /** @brief BIOS image version
     *
     * @return string of the BIOS image version
     */
    virtual std::string imageVersion() = 0;

    /** @brief Persistent regions in the BIOS image
     *
     * @return a list of persistent image region
     */
    virtual std::vector<struct image_region> persistentRegions() = 0;

    /** @brief Verify the BIOS image with the public key
     *
     * @param[in] prod   If true, use the prod key for validation. If not, use
     *   dev key.
     *
     * @return the image is validated with the given key
     */
    virtual bool verify(bool prod) = 0;

    /** @brief The BIOS image is prod signed
     *
     * @return true if the BIOS image is prod signed
     */
    bool prodImage()
    {
        return prod;
    }

    /** @brief SHA256 of the CR51 descriptor
     *
     * @return the HASH of the CR51 descriptor
     */
    std::vector<uint8_t> descriptorHash()
    {
        return hash;
    }

  protected:
    bool prod;
    std::vector<uint8_t> hash;
};

/** @class Cr51Impl
 *  @brief implementation for Cr51 helper
 */
class Cr51Impl : public Cr51
{
  public:
    Cr51Impl() : Cr51(){};
    virtual ~Cr51Impl() = default;

    bool validateImage(std::string_view file, uint32_t size,
                       Config::Key keys) override;

    std::string imageVersion() override;

    std::vector<struct image_region> persistentRegions() override;

    bool verify(bool prod) override;

  private:
    /** @brief Read the image file and populate the buffer
     *
     * @return true if there are no error
     */
    bool readCr51(bool prod);

    std::string file;
    uint32_t size;
    flashupdate::Config::Key keys;
    std::string version;
    std::vector<struct image_region> regions;
    bool valid = false;

    static constexpr int kSignatureRsa4096Pkcs15KeyLength = 512;

    struct hash_ctx shaContext;
    struct libcr51sign_ctx context;
};

} // namespace cr51
} // namespace flashupdate