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

#include <flasher/mod.hpp>
#include <flashupdate/config.hpp>
#include <flashupdate/cr51.hpp>
#include <flashupdate/flash.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace flashupdate
{

/** @class Args
 *  @brief Supported Argument for flashupdate
 */
class Args
{
  public:
    enum class Op
    {
        HashDescriptor,
        Info,
        InjectPersistent,
        Read,
        UpdateStagedVersion,
        UpdateState,
        ValidateConfig,
        Write,
    };
    Op op;
    uint8_t verbose = 0;
    std::optional<flasher::ModArgs> file;
    std::string_view state;

    // Read/Write Command
    bool keepMux = false;
    bool primary = false;
    uint8_t stagingIndex = 0;

    // Info Command
    bool checkActiveVersion = false;
    bool checkStageVersion = false;
    bool checkStageState = false;
    bool otherInfo = false;
    bool cleanOutput = false;

    // Json Config File
    std::optional<std::string> configFile;

    Config config;

    // CR51 Helper
    std::unique_ptr<cr51::Cr51Impl> cr51HelperPtr;
    cr51::Cr51Impl* cr51Helper;

    void setCr51Helper(cr51::Cr51* cr51Helper);

    // Flash Helper
    std::unique_ptr<flash::Flash> flashHelperPtr;
    flash::Flash* flashHelper;

    void setFlashHelper(flash::Flash* flashHelper);

    Args(int argc, char* argv[]);
    Args();

    static std::function<void(const char* arg0)> printHelp;
    static Args argsOrHelp(int argc, char* argv[]);

  private:
    /** @brief Help message for Info command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printInfoHelp(const char* arg0);

    /** @brief Help message for InjectPersistent command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printInjectPersistentHelp(const char* arg0);

    /** @brief Help message for HashDescriptor command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printHashDescriptorHelp(const char* arg0);

    /** @brief Help message for Write command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printWriteHelp(const char* arg0);

    /** @brief Help message for Read command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printReadHelp(const char* arg0);

    /** @brief Help message for UpdateStagedVersion command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printUpdateStagedVersionHelp(const char* arg0);

    /** @brief Help message for UpdateState command
     *
     * @param[in] arg0  flashupdate binary name
     */
    static void printUpdateStateHelp(const char* arg0);
};

} // namespace flashupdate
