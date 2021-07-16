#pragma once
#include <fmt/format.h>

#include <stdplus/exception.hpp>
#include <stdplus/types.hpp>
#include <stdplus/fd/intf.hpp>

#include <sys/utsname.h>
#include <utility>

namespace flasher
{

template <typename Fun, typename Fd, typename Byte, typename... Args>
static void opAtExact(const char* name, Fun&& fun, Fd& fd,
                    stdplus::span<Byte> data, size_t offset, Args&&... args)
{
    while (data.size() > 0)
    {
        auto ret = (fd.*fun)(data, offset, std::forward<Args>(args)...);
        if (ret.size() == 0)
        {
            throw stdplus::exception::WouldBlock(
                fmt::format("{} missing {}B", name, data.size()));
        }
	offset += ret.size();
        data = data.subspan(ret.size());
    }
}

template <typename Fun, typename Fd, typename Byte, typename... Args>
static stdplus::span<Byte> opAt(Fun&& fun, Fd& fd, size_t &cur_offset, stdplus::span<Byte> data, size_t new_offset, Args&&... args)
{
    if (cur_offset != new_offset)
    {
	fd.lseek(new_offset, stdplus::fd::Whence::Set);
	cur_offset = new_offset;
    }
    auto ret = (fd.*fun)(data, std::forward<Args>(args)...);
    cur_offset += ret.size();
    return ret;
}

} // namespace flasher
