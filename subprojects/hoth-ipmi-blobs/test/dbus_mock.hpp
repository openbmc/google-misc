#pragma once

#include "dbus.hpp"

#include <ipmid/api.h>
#include <systemd/sd-bus.h>

#include <string_view>

#include <gmock/gmock.h>

sd_bus* ipmid_get_sd_bus_connection()
{
    return nullptr;
}

namespace ipmi_hoth
{
namespace internal
{

class DbusMock : public virtual Dbus
{
  public:
    MOCK_METHOD0(getHothdMapping, SubTreeMapping());
    MOCK_METHOD1(pingHothd, bool(std::string_view));
};

} // namespace internal

} // namespace ipmi_hoth
