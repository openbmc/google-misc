#include <flasher/file.hpp>
#include <flasher/util.hpp>

namespace flasher
{

void File::readAtExact(stdplus::span<std::byte> data, size_t offset)
{
    opAtExact("File readAtExact", &File::readAt, *this, data, offset);
}
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
