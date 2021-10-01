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

#include <fmt/format.h>
#include <getopt.h>

#include <flasher/device.hpp>
#include <flashupdate/args.hpp>
#include <flashupdate/config.hpp>
#include <flashupdate/cr51.hpp>
#include <flashupdate/flash.hpp>
#include <flashupdate/info.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace flashupdate
{

const std::unordered_map<std::string_view, Args::Op> stringToOp = {
    {"hash_descriptor", Args::Op::HashDescriptor},
    {"inject_persistent", Args::Op::InjectPersistent},
    {"info", Args::Op::Info},
    {"read", Args::Op::Read},
    {"update_staged_version", Args::Op::UpdateStagedVersion},
    {"update_state", Args::Op::UpdateState},
    {"validate_config", Args::Op::ValidateConfig},
    {"write", Args::Op::Write},
};

void Args::setCr51Helper(cr51::Cr51* cr51Helper)
{
    this->cr51Helper = reinterpret_cast<cr51::Cr51Impl*>(cr51Helper);
}

void Args::setFlashHelper(flash::Flash* flashHelper)
{
    this->flashHelper = flashHelper;
}

Args::Args()
{
    cr51HelperPtr = std::make_unique<cr51::Cr51Impl>();
    cr51Helper = cr51HelperPtr.get();

    flashHelperPtr = std::make_unique<flash::Flash>();
    flashHelper = flashHelperPtr.get();
}

Args::Args(int argc, char* argv[]) : Args()
{
    static const char opts[] = ":j:i:acfkSsv";
    static const struct option longopts[] = {
        {"active_version", no_argument, nullptr, 'a'},
        {"clean_output", no_argument, nullptr, 'c'},
        {"config", required_argument, nullptr, 'j'},
        {"stage_state", no_argument, nullptr, 'S'},
        {"stage_version", no_argument, nullptr, 's'},
        {"staging_index", required_argument, nullptr, 'i'},
        {"keep_mux", no_argument, nullptr, 'k'},
        {"verbose", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0},
    };

    int c;
    optind = 0;
    while ((c = getopt_long(argc, argv, opts, longopts, nullptr)) > 0)
    {
        switch (c)
        {
            case 'a':
                checkActiveVersion = true;
                break;
            case 's':
                checkStageVersion = true;
                break;
            case 'S':
                checkStageState = true;
                break;
            case 'v':
                verbose++;
                break;
            case 'c':
                cleanOutput = true;
                break;
            case 'k':
                keepMux = true;
                break;
            case 'i':
                stagingIndex = std::atoi(optarg);
                break;
            case 'j':
                configFile.emplace(optarg);
                break;
            case ':':
                throw std::runtime_error(
                    fmt::format("Missing argument for `{}`", argv[optind - 1]));
                break;
            default:
                throw std::runtime_error(fmt::format(
                    "Invalid command line argument `{}`", argv[optind - 1]));
        }
    }

    // Enabled all UpdateInfo output when using `Info` command
    // If no specific targeted information, print out all avaliable information
    // for general debugging purpose.
    if (!checkActiveVersion && !checkStageVersion && !checkStageState)
    {
        checkActiveVersion = true;
        checkStageVersion = true;
        checkStageState = true;
        otherInfo = true;
    }

    if (optind == argc)
    {
        throw std::runtime_error("Mising flashupdate operation");
    }
    auto it = stringToOp.find(argv[optind]);
    if (it == stringToOp.end())
    {
        throw std::runtime_error(
            fmt::format("Invalid operation: {}", argv[optind]));
    }

    optind++;
    op = it->second;
    switch (op)
    {
        case Args::Op::ValidateConfig:
            break;
        case Args::Op::HashDescriptor:
            printHelp = [this](const char* arg0) {
                printInjectPersistentHelp(arg0);
            };

            if (optind + 1 != argc)
            {
                throw std::runtime_error("Must specify FILE");
            }
            file.emplace(argv[optind]);

            break;
        case Args::Op::InjectPersistent:
            printHelp = [this](const char* arg0) {
                printInjectPersistentHelp(arg0);
            };

            if (optind + 1 != argc)
            {
                throw std::runtime_error("Must specify FILE");
            }
            file.emplace(argv[optind]);

            break;
        case Args::Op::Read:
            printHelp = [this](const char* arg0) { printReadHelp(arg0); };
            if (optind + 2 != argc)
            {
                throw std::runtime_error("Must specify FLASH_TYPE and FILE");
            }

            if (strcmp(argv[optind], "primary") &&
                strcmp(argv[optind], "secondary"))
            {
                throw std::runtime_error(
                    "FLASH_TYPE must be primary or secondary");
            }
            primary = !strcmp(argv[optind], "primary");
            file.emplace(argv[optind + 1]);
            break;
        case Args::Op::Write:
            printHelp = [this](const char* arg0) { printWriteHelp(arg0); };

            if (optind + 2 != argc)
            {
                throw std::runtime_error("Must specify FILE and FLASH_TYPE");
            }

            if (strcmp(argv[optind + 1], "primary") &&
                strcmp(argv[optind + 1], "secondary"))
            {
                throw std::runtime_error(
                    "FLASH_TYPE must be primary or secondary");
            }
            file.emplace(argv[optind]);
            primary = !strcmp(argv[optind + 1], "primary");

            break;
        case Args::Op::UpdateState:
            printHelp = [this](const char* arg0) {
                printUpdateStateHelp(arg0);
            };

            if (optind + 1 != argc)
            {
                throw std::runtime_error("Must specify STATE");
            }

            state = argv[optind];
            break;
        case Args::Op::UpdateStagedVersion:
            printHelp = [this](const char* arg0) {
                printUpdateStagedVersionHelp(arg0);
            };

            if (optind + 1 != argc)
            {
                throw std::runtime_error("Must specify FILE");
            }
            file.emplace(argv[optind]);
            break;
        case Args::Op::Info:
            printHelp = [this](const char* arg0) { printInfoHelp(arg0); };
            break;
    };

    config = createConfig(configFile, stagingIndex);
}

void Args::printInjectPersistentHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... inject_persistent FILE\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} inject_persistent image.bin\n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printHashDescriptorHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... hash_descriptor FILE\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} inject_persistent image.bin\n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printUpdateStateHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... update_state STATE\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "STATE options\n");
    fmt::print(stderr, info::listStates());

    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} update_state STAGED\n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printUpdateStagedVersionHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... update_stage_version FILE\n",
               arg0);
    fmt::print(stderr, "     Note: This should only be used by RAM based "
                       "update to manually update the staged version.\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} update_state image.bin \n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printWriteHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... write FILE FLASH_TYPE\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "FLASH_TYPE options\n");
    fmt::print(stderr, "  `primary`\n");
    fmt::print(stderr, "  `secondary`\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Optional Arguments for `write` command:\n");
    fmt::print(stderr, "  -i, --staging_index   Index to select the secondary "
                       "partition to write to.\n");
    fmt::print(stderr, "  -k, --keep_mux        Keep the mux state to select "
                       "the current flash.\n");

    fmt::print(stderr, "Ex: {} write image.bin primary\n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printReadHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... read FLASH_TYPE FILE\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Optional Arguments for `read` command:\n");
    fmt::print(stderr, "  -i, --staging_index   Index to select the secondary "
                       "partition to read from.\n");

    fmt::print(stderr, "FLASH_TYPE options\n");
    fmt::print(stderr, "  `primary`\n");
    fmt::print(stderr, "  `secondary`\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} read primary image.bin\n", arg0);
    fmt::print(stderr, "\n");
}

void Args::printInfoHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... info\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Optional Arguments for `info` command:\n");
    fmt::print(stderr, "  -a, --active_version   Print the active version with "
                       "the `info command\n");
    fmt::print(stderr, "  -s, --stage_version    Print the stage version with "
                       "the `info` command\n");
    fmt::print(stderr, "  -S, --stage_state      Print the Staged "
                       "stage of the BIOS image.\n");
    fmt::print(stderr, "  -c, --clean_output     Print the `info` "
                       "message with no prefixes\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} info -avS\n", arg0);
    fmt::print(stderr, "\n");
}

std::function<void(const char* arg0)> Args::printHelp = [](const char* arg0) {
    fmt::print(stderr, "Usage: {} [OPTION]... inject_persistent FILE\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... read FLASH_TYPE FILE\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... write FILE FLASH_TYPE\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... update_state STATE\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... update_staged_version FILE\n",
               arg0);
    fmt::print(stderr, "   or: {} [OPTION]... info\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... hash_descriptor\n", arg0);
    fmt::print(stderr, "\n");

    fmt::print(stderr, "General Optional Arguments:\n");
    fmt::print(stderr, "  -v, --verbose          Increases the verbosity "
                       "level of error message output\n");
    fmt::print(stderr, "  -j, --config[=JSON]     Path for json config. "
                       "(default to /usr/share/bios-update/config.json)\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} inject_persistent image.bin\n", arg0);
    fmt::print(stderr, "Ex: {} read primary image.bin\n", arg0);
    fmt::print(stderr, "Ex: {} write image.bin primary\n", arg0);
    fmt::print(stderr, "Ex: {} update_state STAGED\n", arg0);
    fmt::print(stderr, "Ex: {} update_staged_version image.bin\n", arg0);
    fmt::print(stderr, "Ex: {} info\n", arg0);
    fmt::print(stderr, "Ex: {} hash_descriptor image.bin\n", arg0);
    fmt::print(stderr, "\n");
};

Args Args::argsOrHelp(int argc, char* argv[])
{
    try
    {
        return Args(argc, argv);
    }
    catch (...)
    {
        printHelp(argv[0]);
        throw;
    }
}

} // namespace flashupdate
