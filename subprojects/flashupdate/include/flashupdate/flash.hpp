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

#include <flashupdate/args.hpp>
#include <flashupdate/logging.hpp>

#include <string>
#include <utility>

namespace flashupdate
{

namespace flash
{

/** @class Flash
 *  @brief Helper for getting Flash related information.
 */
class Flash
{
  public:
    /** @brief Constructor Flash helper
     *
     * @param[in] config    Flashupdate configuration.
     * @param[in] keepMux   Keep the mux enabled.
     */
    Flash(Config config, bool keepMux);
    ~Flash();

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
    std::optional<std::pair<std::string, uint32_t>> getFlash(bool primary);

  private:
    Config config;
    bool keepMux;
    void cleanup();
};

} // namespace flash
} // namespace flashupdate
