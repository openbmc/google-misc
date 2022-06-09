#include <algorithm>
#include <map>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <xyz/openbmc_project/Time/Boot/Statistic/server.hpp>



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
namespace server
{

Statistic::Statistic(bus_t& bus, const char* path)
        : _xyz_openbmc_project_Time_Boot_Statistic_interface(
                bus, path, interface, _vtable, this), _intf(bus.getInterface())
{
}

Statistic::Statistic(bus_t& bus, const char* path,
                           const std::map<std::string, PropertiesVariant>& vals,
                           bool skipSignal)
        : Statistic(bus, path)
{
    for (const auto& v : vals)
    {
        setPropertyByName(v.first, v.second, skipSignal);
    }
}




auto Statistic::internalRebootCount() const ->
        uint32_t
{
    return _internalRebootCount;
}

int Statistic::_callback_get_InternalRebootCount(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Statistic*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->internalRebootCount();
                    }
                ));
    }
}

auto Statistic::internalRebootCount(uint32_t value,
                                         bool skipSignal) ->
        uint32_t
{
    if (_internalRebootCount != value)
    {
        _internalRebootCount = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Statistic_interface.property_changed("InternalRebootCount");
        }
    }

    return _internalRebootCount;
}

auto Statistic::internalRebootCount(uint32_t val) ->
        uint32_t
{
    return internalRebootCount(val, false);
}

int Statistic::_callback_set_InternalRebootCount(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Statistic*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                value, o->_intf, error,
                std::function(
                    [=](uint32_t&& arg)
                    {
                        o->internalRebootCount(std::move(arg));
                    }
                ));
    }

    return true;
}

namespace details
{
namespace Statistic
{
static const auto _property_InternalRebootCount =
    utility::tuple_to_array(message::types::type_id<
            uint32_t>());
}
}

auto Statistic::powerCycleType() const ->
        PowerCycleType
{
    return _powerCycleType;
}

int Statistic::_callback_get_PowerCycleType(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Statistic*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->powerCycleType();
                    }
                ));
    }
}

auto Statistic::powerCycleType(PowerCycleType value,
                                         bool skipSignal) ->
        PowerCycleType
{
    if (_powerCycleType != value)
    {
        _powerCycleType = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Statistic_interface.property_changed("PowerCycleType");
        }
    }

    return _powerCycleType;
}

auto Statistic::powerCycleType(PowerCycleType val) ->
        PowerCycleType
{
    return powerCycleType(val, false);
}

int Statistic::_callback_set_PowerCycleType(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Statistic*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                value, o->_intf, error,
                std::function(
                    [=](PowerCycleType&& arg)
                    {
                        o->powerCycleType(std::move(arg));
                    }
                ));
    }

    return true;
}

namespace details
{
namespace Statistic
{
static const auto _property_PowerCycleType =
    utility::tuple_to_array(message::types::type_id<
            sdbusplus::xyz::openbmc_project::Time::Boot::server::Statistic::PowerCycleType>());
}
}

void Statistic::setPropertyByName(const std::string& _name,
                                     const PropertiesVariant& val,
                                     bool skipSignal)
{
    if (_name == "InternalRebootCount")
    {
        auto& v = std::get<uint32_t>(val);
        internalRebootCount(v, skipSignal);
        return;
    }
    if (_name == "PowerCycleType")
    {
        auto& v = std::get<PowerCycleType>(val);
        powerCycleType(v, skipSignal);
        return;
    }
}

auto Statistic::getPropertyByName(const std::string& _name) ->
        PropertiesVariant
{
    if (_name == "InternalRebootCount")
    {
        return internalRebootCount();
    }
    if (_name == "PowerCycleType")
    {
        return powerCycleType();
    }

    return PropertiesVariant();
}


namespace
{
/** String to enum mapping for Statistic::PowerCycleType */
static const std::tuple<const char*, Statistic::PowerCycleType> mappingStatisticPowerCycleType[] =
        {
            std::make_tuple( "xyz.openbmc_project.Time.Boot.Statistic.PowerCycleType.ACPowerCycle",                 Statistic::PowerCycleType::ACPowerCycle ),
            std::make_tuple( "xyz.openbmc_project.Time.Boot.Statistic.PowerCycleType.DCPowerCycle",                 Statistic::PowerCycleType::DCPowerCycle ),
        };

} // anonymous namespace

auto Statistic::convertStringToPowerCycleType(const std::string& s) noexcept ->
        std::optional<PowerCycleType>
{
    auto i = std::find_if(
            std::begin(mappingStatisticPowerCycleType),
            std::end(mappingStatisticPowerCycleType),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingStatisticPowerCycleType) == i)
    {
        return std::nullopt;
    }
    else
    {
        return std::get<1>(*i);
    }
}

auto Statistic::convertPowerCycleTypeFromString(const std::string& s) ->
        PowerCycleType
{
    auto r = convertStringToPowerCycleType(s);

    if (!r)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return *r;
    }
}

std::string Statistic::convertPowerCycleTypeToString(Statistic::PowerCycleType v)
{
    auto i = std::find_if(
            std::begin(mappingStatisticPowerCycleType),
            std::end(mappingStatisticPowerCycleType),
            [v](auto& e){ return v == std::get<1>(e); });
    if (i == std::end(mappingStatisticPowerCycleType))
    {
        throw std::invalid_argument(std::to_string(static_cast<int>(v)));
    }
    return std::get<0>(*i);
}

const vtable_t Statistic::_vtable[] = {
    vtable::start(),
    vtable::property("InternalRebootCount",
                     details::Statistic::_property_InternalRebootCount
                        .data(),
                     _callback_get_InternalRebootCount,
                     _callback_set_InternalRebootCount,
                     vtable::property_::emits_change),
    vtable::property("PowerCycleType",
                     details::Statistic::_property_PowerCycleType
                        .data(),
                     _callback_get_PowerCycleType,
                     _callback_set_PowerCycleType,
                     vtable::property_::emits_change),
    vtable::end()
};

} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

