#include <fcntl.h>
#include <fmt/format.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>

#include <flasher/device/mtd.hpp>
#include <flasher/util.hpp>
#include <stdplus/fd/create.hpp>

#include <cstring>
#include <stdexcept>
#include <utility>

namespace flasher
{
namespace device
{

using stdplus::fd::Whence;

Mtd::Mtd(stdplus::ManagedFd&& fd) :
    Device(buildDeviceInfo(fd)), fd(std::move(fd)), offset(this->fd.lseek(0, Whence::Cur))
{}

stdplus::span<std::byte> Mtd::readAt(stdplus::span<std::byte> buf, size_t offset)
{
    return opAt(&stdplus::Fd::read, fd, this->offset, buf, offset);
}

stdplus::span<const std::byte> Mtd::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    return opAt(&stdplus::Fd::write, fd, this->offset, data, offset);
}

void Mtd::eraseBlocks(size_t idx, size_t num)
{
    struct erase_info_user erase;
    memset(&erase, 0, sizeof(erase));
    auto erase_size = getEraseSize();
    erase.length = erase_size * num;
    erase.start = erase_size * idx;
    fd.ioctl(MEMERASE, &erase);
}

Device::DeviceInfo Mtd::buildDeviceInfo(stdplus::Fd& fd)
{
    mtd_info_t info;
    fd.ioctl(MEMGETINFO, &info);

    DeviceInfo ret;
    switch (info.type)
    {
        case MTD_NORFLASH:
            ret.type = Type::Nor;
            break;
        default:
            throw std::invalid_argument(
                fmt::format("Unknown MTD type {:#x}", info.type));
    }
    ret.size = info.size;
    ret.erase_size = info.erasesize;
    return ret;
}

class MtdType : public DeviceType
{
  public:
    std::unique_ptr<Device> open(const ModArgs& args) override
    {
        if (args.arr.size() != 2)
        {
            throw std::invalid_argument("Requires a single file argument");
        }
        return std::make_unique<Mtd>(stdplus::fd::open(
            args.arr[1].c_str(),
            stdplus::fd::OpenFlags(stdplus::fd::OpenAccess::ReadWrite)));
    }

    void printHelp() const override
    {
        fmt::print(stderr, "  `mtd` device\n");
        fmt::print(stderr, "    FILE             required  MTD device filename\n");
    }
};

void registerMtd() __attribute__((constructor));
void registerMtd()
{
    deviceTypes.emplace("mtd", std::make_unique<MtdType>());
}

void unregisterMtd() __attribute__((destructor));
void unregisterMtd()
{
    deviceTypes.erase("mtd");
}

} // namespace device
} // namespace flasher
