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
#include <flashupdate/info.hpp>

#include <memory>
#include <string_view>

namespace flashupdate
{
namespace ops
{

/** @brief Operation for info command
 *
 * @param[in] args  User input argument
 *
 * @return string copy of the UpdateInfo
 */
std::string info(const Args& args);

/** @brief Operation for inject_persistent command
 *
 * @param[in] args  User input argument
 */
void injectPersistent(const Args& args);

/** @brief Operation for hash_descriptor command
 *
 * @param[in] args  User input argument
 *
 * @return string of the CR51 descriptor hash
 */
std::string hashDescriptor(const Args& args);

/** @brief Operation for read command
 *
 * @param[in] args  User input argument
 */
void read(const Args& args);

/** @brief Operation for update_staged_version command
 *
 * @param[in] args  User input argument
 *
 * @return string copy of the UpdateInfo after the update
 */
std::string updateStagedVersion(const Args& args);

/** @brief Operation for update_state command
 *
 * @param[in] args  User input argument
 *
 * @return string copy of the UpdateInfo after the update
 */
std::string updateState(const Args& args);

/** @brief Operation for write command
 *
 * @param[in] args  User input argument
 *
 * The write command is divided into two different types.
 *   - Writing to secondary flash
 *   - Writing to primary flash

 * Writing to secondary flash will do the following:
 *   - Validate the CR51 for the installing firmware
 *   - Write to the flash
 *   - Validate the flash
 *   - Create the hash of the CR51 descriptor
 *   - Update the Update Status to the EEPROM
 *     - Staged Version
 *     - Update State -> Staged
 *     - Save the hash of the CR51 descriptor
 * 
 * Writing to primary flash will do the following:
 *   - Validate the CR51 for the installing firmware
 *   - Validate the CR51 for the running firmware
 *   - Check the signature type of both firmware
 *     - Block dev to prod updates
 *   - Fetch the hash of the CR51 descriptor
 *     - Compare it to the hash of the CR51 descriptor for installing
 *       firmware. Exit if it does not match.
 *   - Write to the flash
 *   - Validate the flash
 *   - Create the hash of the CR51 descriptor
 *   - Update the Update Status to the EEPROM
 *     - Active Version
 *     - Update State -> UPDATED
 */
void write(const Args& args);

} // namespace ops
} // namespace flashupdate
