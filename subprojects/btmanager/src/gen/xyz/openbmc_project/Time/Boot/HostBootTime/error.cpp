#include <xyz/openbmc_project/Time/Boot/HostBootTime/error.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Time
{
namespace Boot
{
namespace HostBootTime
{
namespace Error
{
const char* UnsupportedTimepoint::name() const noexcept
{
    return errName;
}
const char* UnsupportedTimepoint::description() const noexcept
{
    return errDesc;
}
const char* UnsupportedTimepoint::what() const noexcept
{
    return errWhat;
}
const char* WrongOrder::name() const noexcept
{
    return errName;
}
const char* WrongOrder::description() const noexcept
{
    return errDesc;
}
const char* WrongOrder::what() const noexcept
{
    return errWhat;
}

} // namespace Error
} // namespace HostBootTime
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

