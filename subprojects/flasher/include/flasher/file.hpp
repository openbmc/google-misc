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
#include <flasher/reader.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/types.hpp>

#include <cstddef>

namespace flasher
{

class File : public Reader
{
  public:
    virtual void truncate(size_t new_size) = 0;
    virtual stdplus::span<const std::byte>
        writeAt(stdplus::span<const std::byte> data, size_t offset) = 0;

    void readAtExact(stdplus::span<std::byte> data, size_t offset);
    void writeAtExact(stdplus::span<const std::byte> data, size_t offset);
};

class FileType : public ModType<File>
{
  public:
    using OpenFlags = stdplus::fd::OpenFlags;

    virtual std::unique_ptr<File> open(const ModArgs& args,
                                       OpenFlags flags) = 0;
};

extern ModTypeMap<FileType> fileTypes;
std::unique_ptr<File> openFile(ModArgs& args, FileType::OpenFlags flags);

} // namespace flasher
