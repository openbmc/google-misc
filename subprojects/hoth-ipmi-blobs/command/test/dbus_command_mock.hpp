#pragma once

#include "dbus_command.hpp"
#include "dbus_mock.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

namespace ipmi_hoth
{
namespace internal
{

class DbusCommandMock : public DbusCommand, public DbusMock
{
  public:
    MOCK_METHOD(stdplus::Cancel, SendHostCommand,
                (std::string_view, const std::vector<uint8_t>&, Cb&&),
                (override));
};

} // namespace internal

} // namespace ipmi_hoth
