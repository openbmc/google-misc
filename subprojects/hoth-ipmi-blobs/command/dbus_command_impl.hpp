#pragma once

#include "dbus_command.hpp"
#include "dbus_impl.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

/** @class DbusImpl
 *  @brief D-Bus concrete implementation
 *  @details Pass through all calls to the default D-Bus instance
 */
class DbusCommandImpl : public DbusCommand, public DbusImpl
{
  public:
    stdplus::Cancel SendHostCommand(std::string_view hothId,
                                    const std::vector<uint8_t>&, Cb&&) override;
};

} // namespace internal

} // namespace ipmi_hoth
