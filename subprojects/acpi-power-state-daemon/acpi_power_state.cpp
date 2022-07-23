// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * Based off of sdbusplus:/example/calculator-server.cpp
 */
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Control/Power/ACPIPowerState/server.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <string>

constexpr auto hostS5Unit = "host-s5-state.target";
constexpr auto hostS0Unit = "host-s0-state.target";

constexpr auto systemdBusName = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdInterface = "org.freedesktop.systemd1.Manager";

constexpr auto acpiObjPath =
    "/xyz/openbmc_project/control/host0/acpi_power_state";
constexpr auto acpiInterface =
    "xyz.openbmc_project.Control.Power.ACPIPowerState";

using ACPIPowerStateInherit = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Control::Power::server::ACPIPowerState>;

// Pulled and modified from arcadia-leds/poll_gpio.cpp
static void startSystemdUnit(sdbusplus::bus_t& bus, const std::string& unit)
{
    auto method = bus.new_method_call(systemdBusName, systemdPath,
                                      systemdInterface, "StartUnit");
    method.append(unit, "replace");
    bus.call(method);
}

struct ACPIPowerState : ACPIPowerStateInherit
{
    // Keep track of the bus for starting/stopping systemd units
    sdbusplus::bus_t& Bus;

    ACPIPowerState(sdbusplus::bus_t& bus, const char* path) :
        ACPIPowerStateInherit(bus, path), Bus(bus)
    {}

    ACPI sysACPIStatus(ACPI value)
    {
        std::cout << "State change "
                  << ACPIPowerStateInherit::convertACPIToString(value)
                  << std::endl;

        switch (value)
        {
            case ACPI::S5_G2:
                std::cout << "Entered S5" << std::endl;
                startSystemdUnit(Bus, hostS5Unit);
                break;
            case ACPI::S0_G0_D0:
                std::cout << "Entered S0" << std::endl;
                startSystemdUnit(Bus, hostS0Unit);
                break;
            default:
                break;
        }

        return ACPIPowerStateInherit::sysACPIStatus(value);
    }
};

int main()
{

    auto b = sdbusplus::bus::new_default();
    sdbusplus::server::manager_t m{b, acpiObjPath};

    // Reserve the dbus service for ACPI Power state changes coming from the
    // BIOS
    b.request_name(acpiInterface);

    ACPIPowerState aps{b, acpiObjPath};

    // Handle dbus processing forever.
    for (;;)
    {
        b.process_discard(); // discard any unhandled messages
        b.wait();
    }

    return 1;
}
