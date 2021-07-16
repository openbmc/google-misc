#pragma once
#include <flasher/mod.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/types.hpp>

#include <cstddef>

namespace flasher
{

class File
{
  public:
    virtual ~File() = default;

    virtual stdplus::span<std::byte> readAt(stdplus::span<std::byte> buf,
                                            size_t offset) = 0;
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
