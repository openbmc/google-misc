#pragma once

#include "dbus_command_mock.hpp"
#include "hoth_command.hpp"

#include <stdplus/util/string.hpp>

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace ipmi_hoth
{

class HothCommandTest : public ::testing::Test
{
  protected:
    explicit HothCommandTest() : hvn(&dbus)
    {}

    // dbus mock object
    testing::StrictMock<internal::DbusCommandMock> dbus;

    HothCommandBlobHandler hvn;

    const uint16_t session_ = 0;
    const std::string legacyPath = stdplus::util::strCat(
        HothCommandBlobHandler::pathPrefix, hvn.pathSuffix());
    const std::string name = "prologue";
    const std::string namedPath = stdplus::util::strCat(
        HothCommandBlobHandler::pathPrefix, name, "/", hvn.pathSuffix());
};

} // namespace ipmi_hoth
