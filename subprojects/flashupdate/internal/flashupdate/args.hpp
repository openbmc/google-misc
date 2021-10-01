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

#include <cstdint>
#include <functional>
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
        Empty,
        HashDescriptor,
        Info,
        InjectPersistent,
        Read,
        UpdateStagedVersion,
        UpdateState,
        Write,
    };
    Op op;
    uint8_t verbose = 0;
    std::optional<flasher::ModArgs> file;
    std::string_view state;

    // Read/Write Command
    bool keepMux = false;
    bool primary;
    uint8_t stagingIndex = 0;

    // Info Command
    bool checkActiveVersion = false;
    bool checkStageVersion = false;
    bool checkStageState = false;
    bool otherInfo = false;
    bool cleanOutput = false;

    Args(int argc, char* argv[]);

    static std::function<void(const char* arg0)> printHelp;
    static Args argsOrHelp(int argc, char* argv[]);

  private:
    static void printInfoHelp(const char* arg0);
    static void printInjectPersistentHelp(const char* arg0);
    static void printHashDescriptorHelp(const char* arg0);
    static void printWriteHelp(const char* arg0);
    static void printReadHelp(const char* arg0);
    static void printUpdateStagedVersionHelp(const char* arg0);
    static void printUpdateStateHelp(const char* arg0);
};

} // namespace flashupdate
