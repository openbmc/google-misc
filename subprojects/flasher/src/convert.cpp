#include <fmt/format.h>

#include <cstdlib>
#include <stdexcept>

namespace flasher
{

uint32_t toUint32(const char* s)
{
    char* end;
    errno = 0;
    uint32_t ret = std::strtoul(s, &end, 0);
    if (errno || end[0] != '\0' || end == s)
    {
        throw std::runtime_error(fmt::format("Invalid uint32: {}", s));
    }
    return ret;
}

} // namespace flasher
