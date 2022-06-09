#pragma once

#include <cerrno>
#include <sdbusplus/exception.hpp>

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

struct UnsupportedTimepoint final :
        public sdbusplus::exception::generated_exception
{
    static constexpr auto errName = "xyz.openbmc_project.Time.Boot.HostBootTime.Error.UnsupportedTimepoint";
    static constexpr auto errDesc =
            "An unsupported stage was sent.";
    static constexpr auto errWhat =
            "xyz.openbmc_project.Time.Boot.HostBootTime.Error.UnsupportedTimepoint: An unsupported stage was sent.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct WrongOrder final :
        public sdbusplus::exception::generated_exception
{
    static constexpr auto errName = "xyz.openbmc_project.Time.Boot.HostBootTime.Error.WrongOrder";
    static constexpr auto errDesc =
            "Setting the timepoint in a wrong order.";
    static constexpr auto errWhat =
            "xyz.openbmc_project.Time.Boot.HostBootTime.Error.WrongOrder: Setting the timepoint in a wrong order.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace Error
} // namespace HostBootTime
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

