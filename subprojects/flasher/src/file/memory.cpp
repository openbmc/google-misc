#include <flasher/file/memory.hpp>

#include <algorithm>
#include <stdplus/exception.hpp>
#include <cstring>

namespace flasher
{
namespace file
{

stdplus::span<std::byte> Memory::readAt(stdplus::span<std::byte> buf, size_t offset)
{
    if (offset == data.size()) throw stdplus::exception::Eof("readAt");
    if (offset > data.size()) throw std::runtime_error("Read offset out of range");
    auto ret = buf.subspan(0, std::min(buf.size(), data.size() - offset));
    std::memcpy(ret.data(), &data[offset], ret.size());
    return ret;
}

stdplus::span<const std::byte>
    Memory::writeAt(stdplus::span<const std::byte> data, size_t offset)
{
    this->data.resize(std::max(this->data.size(), offset + data.size()));
    std::memcpy(&this->data[offset], data.data(), data.size());
    return stdplus::span<const std::byte>(this->data).subspan(offset, data.size());
}

} // namespace file
} // namespace flasher
