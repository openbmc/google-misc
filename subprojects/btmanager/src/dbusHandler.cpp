#include "dbusHandler.hpp"

#include <btDefinitions.hpp>
#include <xyz/openbmc_project/Time/Boot/HostBootTime/error.hpp>

#include <iostream>

constexpr const bool debug = true;

DbusHandler::DbusHandler(sdbusplus::bus::bus& dbus,
                         const std::string& objPath) :
    sdbusplus::server::object::object<Durations>(dbus, objPath.c_str()),
    sdbusplus::server::object::object<HostBootTime>(dbus, objPath.c_str()),
    sdbusplus::server::object::object<Statistic>(dbus, objPath.c_str()),
    _btsm(nullptr)
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
        _btsm->setDuration(stage, durationMicrosecond, false);
        return SetDurationStates::KeyDurationSet;
    }
    else
    {
        if (_btsm->setDuration(stage, durationMicrosecond, true))
        {
            return SetDurationStates::ExtraDurationSet;
        }
        else
        {
            return SetDurationStates::DurationNotSettable;
        }
    }
}

void DbusHandler::update(const nlohmann::json& j)
{
    using Durations =
        sdbusplus::xyz::openbmc_project::Time::Boot::server::Durations;
    using Statistic =
        sdbusplus::xyz::openbmc_project::Time::Boot::server::Statistic;

    // Durations
    Durations::osUserspaceShutdown(
        j[BTCategory::kDuration].contains(BTDuration::kOSUserDown)
            ? j[BTCategory::kDuration][BTDuration::kOSUserDown].get<uint64_t>()
            : 0);
    Durations::osKernelShutdown(
        j[BTCategory::kDuration].contains(BTDuration::kOSKernelDown)
            ? j[BTCategory::kDuration][BTDuration::kOSKernelDown]
                  .get<uint64_t>()
            : 0);
    Durations::bmcShutdown(
        j[BTCategory::kDuration].contains(BTDuration::kBMCDown)
            ? j[BTCategory::kDuration][BTDuration::kBMCDown].get<uint64_t>()
            : 0);
    Durations::bmc(
        j[BTCategory::kDuration].contains(BTDuration::kBMC)
            ? j[BTCategory::kDuration][BTDuration::kBMC].get<uint64_t>()
            : 0);
    Durations::bios(
        j[BTCategory::kDuration].contains(BTDuration::kBIOS)
            ? j[BTCategory::kDuration][BTDuration::kBIOS].get<uint64_t>()
            : 0);
    Durations::nerfKernel(
        j[BTCategory::kDuration].contains(BTDuration::kNerfKernel)
            ? j[BTCategory::kDuration][BTDuration::kNerfKernel].get<uint64_t>()
            : 0);
    Durations::nerfUserspace(
        j[BTCategory::kDuration].contains(BTDuration::kNerfUser)
            ? j[BTCategory::kDuration][BTDuration::kNerfUser].get<uint64_t>()
            : 0);
    Durations::osKernel(
        j[BTCategory::kDuration].contains(BTDuration::kOSKernel)
            ? j[BTCategory::kDuration][BTDuration::kOSKernel].get<uint64_t>()
            : 0);
    Durations::osUserspace(
        j[BTCategory::kDuration].contains(BTDuration::kOSUser)
            ? j[BTCategory::kDuration][BTDuration::kOSUser].get<uint64_t>()
            : 0);
    Durations::unmeasured(
        j[BTCategory::kDuration].contains(BTDuration::kUnmeasured)
            ? j[BTCategory::kDuration][BTDuration::kUnmeasured].get<uint64_t>()
            : 0);
    Durations::total(
        j[BTCategory::kDuration].contains(BTDuration::kTotal)
            ? j[BTCategory::kDuration][BTDuration::kTotal].get<uint64_t>()
            : 0);
    std::vector<std::tuple<std::string, uint64_t>> extras;
    if (j[BTCategory::kDuration].contains(BTDuration::kExtra))
    {
        for (auto& el : j[BTCategory::kDuration][BTDuration::kExtra].items())
        {
            extras.push_back({el.key(), el.value()});
        }
    }
    Durations::extra(extras);

    // Statistic
    Statistic::internalRebootCount(
        j[BTCategory::kStatistic].contains(BTStatistic::kInternalRebootCount)
            ? j[BTCategory::kStatistic][BTStatistic::kInternalRebootCount]
                  .get<uint16_t>()
            : 0);
    Statistic::powerCycleType(
        j[BTCategory::kStatistic].contains(BTStatistic::kIsACPowerCycle)
            ? (j[BTCategory::kStatistic][BTStatistic::kIsACPowerCycle]
                       .get<bool>()
                   ? PowerCycleType::ACPowerCycle
                   : PowerCycleType::DCPowerCycle)
            : PowerCycleType::ACPowerCycle);
}

void DbusHandler::setStateMachine(std::shared_ptr<BTStateMachine> btsm)
{
    _btsm = btsm;
}
