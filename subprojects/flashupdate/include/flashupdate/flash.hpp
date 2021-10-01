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

#include <flashupdate/config.hpp>

#include <memory>
#include <string>
#include <utility>

namespace flashupdate
{

namespace flash
{

/** @class FlashHelper
 *  @brief Helper functions for Flash
 */
class FlashHelper
{
  public:
    virtual ~FlashHelper() = default;

    /** @brief Read the content in the mtd file
     *
     * @param[in] filename   File to read the data from
     *
     * @return the content of the file in string
     */
    virtual std::string readMtdFile(const std::string& filename);
};

/** @class Flash
 *  @brief Helper for getting Flash related information.
 */
class Flash
{
  public:
    Flash(){};

    /** @brief Constructor Flash helper
     *
     * @param[in] config    Flashupdate configuration.
     * @param[in] keepMux   Keep the mux enabled.
     */
    Flash(Config config, bool keepMux);

    /** @brief Setup Flash helper
     *
     * @param[in] config    Flashupdate configuration.
     * @param[in] keepMux   Keep the mux enabled.
     */
    void setup(Config config, bool keepMux)
    {
        config = config;
        keepMux = keepMux;
    }

    Flash& operator=(Flash&& other) = default;
    virtual ~Flash();

    /** @brief Find the flash partiion for the Flash update
     *
     * The config contains the information needed to determine the flash
     * partition to use. Confirm the configuration is valid and return the
     * device block and size.
     *
     * @param[in] primary   If true, return the information for primary
     *   flash. Otherwise, return secondary flash information.
     *
     * @return pair of device block name and size if valid, std::nullopt
     *   otherwise.
     */
    virtual std::optional<std::pair<std::string, uint32_t>>
        getFlash(bool primary);

    void setFlashHelper(FlashHelper* helper);

  protected:
    /** @brief Cleanup function for flash partition helper
     *
     * Reset the MUX select GPIO and unbound the driver for the flash.
     */
    virtual void cleanup();

  private:
    Config config;
    bool keepMux;

    std::unique_ptr<FlashHelper> helperPtr;
    FlashHelper* helper;
};

} // namespace flash
} // namespace flashupdate
