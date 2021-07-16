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

#include <flasher/file.hpp>
#include <flasher/util.hpp>

namespace flasher
{

void File::writeAtExact(stdplus::span<const std::byte> data, size_t offset)
{
    opAtExact("File writeAtExact", &File::writeAt, *this, data, offset);
}

ModTypeMap<FileType> fileTypes;

std::unique_ptr<File> openFile(ModArgs& args, FileType::OpenFlags flags)
{
    if (args.arr.size() == 1)
    {
        args.arr.insert(args.arr.begin(), "simple");
    }
    return openMod<File>(fileTypes, args, flags);
}

} // namespace flasher
