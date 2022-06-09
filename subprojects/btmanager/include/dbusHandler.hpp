#pragma once

#include "btDefinitions.hpp"
#include "btStateMachine.hpp"

#include <sdbusplus/asio/object_server.hpp>
#include <xyz/openbmc_project/Time/Boot/Durations/server.hpp>
#include <xyz/openbmc_project/Time/Boot/HostBootTime/server.hpp>
#include <xyz/openbmc_project/Time/Boot/Statistic/server.hpp>

#include <cstdint>
#include <memory>
#include <string>

class DbusHandler :
    sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations>,
    sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Time::Boot::server::HostBootTime>,
    sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Time::Boot::server::Statistic>
{
  public:
    DbusHandler(sdbusplus::bus::bus& dbus, const std::string& objPath,
                std::shared_ptr<BTStateMachine> btsm);

    uint64_t notify(uint8_t timepoint) override;
    SetDurationStates setDuration(std::string stage,
                                  uint64_t durationMicrosecond) override;

    void updateDurations(BTDuration durations);
    void updateInternalRebootCount(uint32_t count);
    void updatePowerCycleType(bool isAC);

  private:
    std::shared_ptr<BTStateMachine> _btsm;
};
