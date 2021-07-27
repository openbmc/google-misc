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
#include "cli.hpp"

#include <string>
#include <variant>

namespace google
{
namespace bios_validator
{

CommandLine::CommandLine() : app("Host BIOS Validator")
{
    app.require_subcommand(1);
    auto* validator = app.add_subcommand(
        kValidatorCMD, "validate the embedded image descriptor in "
                       "`BIOS_FILE` using the "
                       "public key KEY_FILE.");
    validator
        ->add_option("bios_file", validatorArgs.biosFilename,
                     "BIOS file to be validated")
        ->required()
        ->check(CLI::ExistingFile);

    validator
        ->add_option("bios_size", validatorArgs.biosFileSize,
                     "Size of the BIOS file")
        ->required()
        ->check(CLI::Range(1u, std::numeric_limits<uint32_t>::max()));

    validator
        ->add_option("key_file", validatorArgs.keyFilename,
                     "Public key in RSA4096_PKCS15 format")
        ->required()
        ->check(CLI::ExistingFile);

    validator->add_option("--write_version", validatorArgs.versionFilename,
                          "File where the image version should be written");
}

int CommandLine::parseArgs(int argc, const char** argv)
{
    CLI11_PARSE(app, argc, argv);
    validatorArgs.writeVersion = !validatorArgs.versionFilename.empty();
    return EXIT_SUCCESS;
}

ValidatorArgs CommandLine::getArgs()
{
    return validatorArgs;
}

bool CommandLine::gotSubcommand(std::string_view cmd)
{
    std::string cmdStr(cmd);
    return app.got_subcommand(cmdStr);
}

} // namespace bios_validator

} // namespace google
