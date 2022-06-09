#include "dbusHandler.hpp"

#include <xyz/openbmc_project/Time/Boot/HostBootTime/error.hpp>

#include <iostream>

DbusHandler::DbusHandler(sdbusplus::bus::bus& dbus, const std::string& objPath,
                         std::shared_ptr<BTStateMachine> btsm) :
    sdbusplus::server::object::object<Durations>(dbus, objPath.c_str()),
    sdbusplus::server::object::object<HostBootTime>(dbus, objPath.c_str()),
    sdbusplus::server::object::object<Statistic>(dbus, objPath.c_str()),
    _btsm(btsm)
{}

uint64_t DbusHandler::notify(uint8_t timepoint)
{
    if (BTTimePoint::dbusOwnedTimePoint.find(timepoint) ==
        BTTimePoint::dbusOwnedTimePoint.end())
    {
        throw sdbusplus::xyz::openbmc_project::Time::Boot::HostBootTime::Error::
            UnsupportedTimepoint();
    }

    SMResult result = _btsm->next(timepoint);
    if (timepoint == BTTimePoint::kOSUserEnd)
    {
        updateDurations(_btsm->getDurations());
        updateInternalRebootCount(_btsm->getInternalRebootCount());
        updatePowerCycleType(_btsm->isACPowerCycle());
    }

    if (result.err == SMErrors::smErrWrongOrder)
    {
        std::cerr << "Error: sending notification in a wrong order"
                  << std::endl;
        throw sdbusplus::xyz::openbmc_project::Time::Boot::HostBootTime::Error::
            WrongOrder();
    }
    return result.value;
}

DbusHandler::SetDurationStates
    DbusHandler::setDuration(std::string stage, uint64_t durationMicrosecond)
{
    if (BTDuration::dbusNotOwnedDuration.find(stage) !=
        BTDuration::dbusNotOwnedDuration.end())
    {
        return SetDurationStates::DurationNotSettable;
    }
    else if (BTDuration::dbusOwnedKeyDuration.find(stage) !=
             BTDuration::dbusOwnedKeyDuration.end())
    {
        _btsm->setDuration(stage, durationMicrosecond);
        return SetDurationStates::KeyDurationSet;
    }
    else
    {
        _btsm->setDuration(stage, durationMicrosecond);
        return SetDurationStates::ExtraDurationSet;
    }
}

void DbusHandler::updateDurations(BTDuration durations)
{
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::
        osUserspaceShutdown(durations.osUserDown);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::
        osKernelShutdown(durations.osKernelDown);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::bmcShutdown(
        durations.bmcDown);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::bmc(
        durations.bmc);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::bios(
        durations.bios);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::nerfKernel(
        durations.nerfKernel);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::
        nerfUserspace(durations.nerfUser);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::osKernel(
        durations.osKernel);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::osUserspace(
        durations.osUser);
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations::unmeasured(
        durations.unmeasured);
}

void DbusHandler::updateInternalRebootCount(uint32_t count)
{
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Statistic::
        internalRebootCount(count);
}

void DbusHandler::updatePowerCycleType(bool isAC)
{
    sdbusplus::xyz::openbmc_project::Time::Boot::server::Statistic::
        powerCycleType(isAC ? PowerCycleType::ACPowerCycle
                            : PowerCycleType::DCPowerCycle);
}
