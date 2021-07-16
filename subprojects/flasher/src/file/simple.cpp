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
#include <flasher/file/simple.hpp>
#include <flasher/util.hpp>
#include <stdplus/fd/create.hpp>

#include <algorithm>
#include <memory>
#include <utility>

namespace flasher
{
namespace file
{

Simple::Simple(stdplus::ManagedFd&& fd) :
    fd(std::move(fd)), size(this->fd.lseek(0, stdplus::fd::Whence::End)),
    offset(this->fd.lseek(0, stdplus::fd::Whence::Cur))
{}

size_t Simple::getSize() const
{
    return size;
}

void Simple::truncate(size_t new_size)
{
    fd.truncate(new_size);
    size = new_size;
}

stdplus::span<std::byte> Simple::readAt(stdplus::span<std::byte> buf,
                                        size_t offset)
{
    return opAt(&stdplus::Fd::read, fd, this->offset, buf, offset);
}

stdplus::span<const std::byte>
    Simple::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    auto ret = opAt(&stdplus::Fd::write, fd, this->offset, data, offset);
    size = std::max(size, offset + ret.size());
    return ret;
}

class SimpleType : public FileType
{
  public:
    std::unique_ptr<File> open(const ModArgs& args,
                               stdplus::fd::OpenFlags flags) override
    {
        if (args.arr.size() != 2)
        {
            throw std::invalid_argument("Requires a single file argument");
        }
        return std::make_unique<Simple>(
            stdplus::fd::open(args.arr[1].c_str(), flags, 0644));
    }

    void printHelp() const override
    {
        fmt::print(stderr, "  `simple` file\n");
        fmt::print(stderr, "    FILENAME         required  simple filename\n");
    }
};

void registerSimple() __attribute__((constructor));
void registerSimple()
{
    fileTypes.emplace("simple", std::make_unique<SimpleType>());
}

void unregisterSimple() __attribute__((destructor));
void unregisterSimple()
{
    fileTypes.erase("simple");
}

} // namespace file
} // namespace flasher
