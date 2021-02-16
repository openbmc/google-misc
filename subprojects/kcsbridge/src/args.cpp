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

#include "args.hpp"

#include <fmt/format.h>
#include <getopt.h>

#include <stdexcept>

namespace kcsbridge
{

Args::Args(int argc, char* argv[])
{
    static const char opts[] = ":c:";
    static const struct option longopts[] = {
        {"channel", required_argument, nullptr, 'c'},
        {nullptr, 0, nullptr, 0},
    };
    int c;
    optind = 0;
    while ((c = getopt_long(argc, argv, opts, longopts, nullptr)) > 0)
    {
        switch (c)
        {
            case 'c':
                channel = optarg;
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
    if (optind != argc)
    {
        throw std::invalid_argument("Requires no additional arguments");
    }
    if (channel == nullptr)
    {
        throw std::invalid_argument("Missing KCS channel");
    }
}

} // namespace kcsbridge
