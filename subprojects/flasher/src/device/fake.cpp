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

#include <flasher/convert.hpp>
#include <flasher/device/fake.hpp>
#include <stdplus/raw.hpp>

#include <optional>
#include <utility>
#include <vector>

namespace flasher
{
namespace device
{

Fake::Fake(File& file, Type type, size_t erase) :
    Device(buildDeviceInfo(file, type, erase)), file(&file),
    erase_contents(erase)
{
    mockErase(erase_contents);
}

stdplus::span<std::byte> Fake::readAt(stdplus::span<std::byte> buf,
                                      size_t offset)
{
    return file->readAt(buf, offset);
}

stdplus::span<const std::byte>
    Fake::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    switch (info.type)
    {
        case Type::Nor:
        {
            std::vector<std::byte> out(data.size());
            file->readAtExact(out, offset);
            for (size_t i = 0; i < out.size(); ++i)
            {
                out[i] &= data[i];
            }
            file->writeAtExact(out, offset);
        }
        break;
        case Type::Simple:
            file->writeAtExact(data, offset);
            break;
    }
    return data;
}

void Fake::eraseBlocks(size_t idx, size_t num)
{
    for (size_t i = idx; i < idx + num; i++)
    {
        file->writeAtExact(stdplus::raw::asSpan<std::byte>(erase_contents),
                           i * erase_contents.size());
    }
}

Device::DeviceInfo Fake::buildDeviceInfo(File& file, Type type, size_t erase)
{
    DeviceInfo ret;
    ret.type = type;
    ret.erase_size = erase;
    ret.size = file.getSize();
    return ret;
}

FakeOwning::FakeOwning(std::unique_ptr<File>&& file, Type type, size_t erase) :
    Fake(*file, type, erase), file(std::move(file))
{}

class FakeType : public DeviceType
{
  public:
    std::unique_ptr<Device> open(const ModArgs& args) override
    {
        if (args.arr.size() != 2)
        {
            throw std::invalid_argument("Requires a single file argument");
        }
        auto it = args.dict.find("type");
        if (it == args.dict.end())
        {
            throw std::invalid_argument("Requires a type to be specified");
        }
        auto type = Device::toType(it->second);
        it = args.dict.find("erase");
        if (it == args.dict.end())
        {
            throw std::invalid_argument("Requires an erase to be specified");
        }
        auto erase = toUint32(it->second.c_str());
        it = args.dict.find("size");
        std::optional<size_t> size;
        if (it != args.dict.end())
        {
            size = toUint32(it->second.c_str());
        }
        stdplus::fd::OpenFlags flags(stdplus::fd::OpenAccess::ReadWrite);
        if (size)
        {
            flags.set(stdplus::fd::OpenFlag::Create);
        }
        ModArgs file_args(args.arr[1]);
        auto file = openFile(file_args, flags);
        if (size)
        {
            file->truncate(*size);
        }
        return std::make_unique<FakeOwning>(std::move(file), type, erase);
    }

    void printHelp() const override
    {
        fmt::print(stderr, "  `fake` device\n");
        fmt::print(stderr, "    type=TYPE        required  The type of flash "
                           "to use (nor, simple)\n");
        fmt::print(
            stderr,
            "    erase=ERASE_SIZE required  The erase size of the flash\n");
        fmt::print(stderr, "    size=SIZE        optional  If specified, "
                           "truncates the file to SIZE\n");
        fmt::print(
            stderr,
            "    FILE             required  Backing file specification\n");
    }
};

void registerFake() __attribute__((constructor));
void registerFake()
{
    deviceTypes.emplace("fake", std::make_unique<FakeType>());
}

void unregisterFake() __attribute__((destructor));
void unregisterFake()
{
    deviceTypes.erase("fake");
}

} // namespace device
} // namespace flasher
