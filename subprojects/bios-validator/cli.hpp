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

#include <CLI/CLI.hpp>

#include <string>
#include <variant>

namespace google
{
namespace bios_validator
{

constexpr char kValidatorCMD[] = "validate";

/**
 * biosFilename: required, positional, file should exist
 * biosFileSize: required, positional, [1, uint32_max]
 * keyFilename: required, positional, file should exist
 */
struct ValidatorArgs
{
    std::string biosFilename;
    unsigned long biosFileSize{0};
    std::string keyFilename;
    std::string versionFilename;

    bool writeVersion = false;
};

class CommandLine
{
  private:
    ValidatorArgs validatorArgs;
    CLI::App app;

  public:
    CommandLine();
    CommandLine(const CommandLine&) = delete;
    CommandLine& operator=(const CommandLine&) = delete;
    CommandLine(CommandLine&&) = delete;
    CommandLine& operator=(CommandLine&&) = delete;

    /**
     * @brief Parse, validate, and store arguments.
     * @return Return EXIT_SUCCESS on success; otherwise, it returns an
     *  error code
     */
    int parseArgs(int argc, const char* argv[]);

    /**
     * @brief Get parsed arguments. Use gotSubcommand() to identify subcommands
     * and types.
     */
    ValidatorArgs getArgs();

    /**
     * @brief Whether it got the input subcommand. It will be used before
     * getArgs() to determine the type of arguments
     */
    bool gotSubcommand(std::string_view cmd);
};
} // namespace bios_validator

} // namespace google
