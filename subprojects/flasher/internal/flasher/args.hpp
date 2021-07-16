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
#include <optional>
#include <vector>

namespace flasher
{

struct Args
{
    enum class Op
    {
        Automatic,
        Read,
        Write,
        Erase,
        Verify,
    };
    Op op;
    bool noread = false;
    bool verify = false;
    std::vector<ModArgs> mutate;
    std::optional<ModArgs> dev;
    std::optional<ModArgs> file;
    size_t dev_offset = 0;
    size_t file_offset = 0;
    size_t max_size = std::numeric_limits<size_t>::max();
    std::optional<size_t> stride;
    uint8_t verbose = 0;

    Args(int argc, char* argv[]);

    static void printHelp(const char* arg0);
    static Args argsOrHelp(int argc, char* argv[]);
};

} // namespace flasher
