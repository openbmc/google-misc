#include <algorithm>
#include <map>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <xyz/openbmc_project/Time/Boot/HostBootTime/server.hpp>

#include <xyz/openbmc_project/Time/Boot/HostBootTime/error.hpp>
#include <xyz/openbmc_project/Time/Boot/HostBootTime/error.hpp>


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

HostBootTime::HostBootTime(bus_t& bus, const char* path)
        : _xyz_openbmc_project_Time_Boot_HostBootTime_interface(
                bus, path, interface, _vtable, this), _intf(bus.getInterface())
{
}


int HostBootTime::_callback_Notify(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto o = static_cast<HostBootTime*>(context);

    try
    {
        return sdbusplus::sdbuspp::method_callback(
                msg, o->_intf, error,
                std::function(
                    [=](uint8_t&& timepoint)
                    {
                        return o->notify(
                                timepoint);
                    }
                ));
    }
    catch(const sdbusplus::xyz::openbmc_project::Time::Boot::HostBootTime::Error::UnsupportedTimepoint& e)
    {
        return o->_intf->sd_bus_error_set(error, e.name(), e.description());
    }
    catch(const sdbusplus::xyz::openbmc_project::Time::Boot::HostBootTime::Error::WrongOrder& e)
    {
        return o->_intf->sd_bus_error_set(error, e.name(), e.description());
    }

    return 0;
}

namespace details
{
namespace HostBootTime
{
static const auto _param_Notify =
        utility::tuple_to_array(message::types::type_id<
                uint8_t>());
static const auto _return_Notify =
        utility::tuple_to_array(message::types::type_id<
                uint64_t>());
}
}

int HostBootTime::_callback_SetDuration(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto o = static_cast<HostBootTime*>(context);

    {
        return sdbusplus::sdbuspp::method_callback(
                msg, o->_intf, error,
                std::function(
                    [=](std::string&& stage, uint64_t&& durationMicrosecond)
                    {
                        return o->setDuration(
                                stage, durationMicrosecond);
                    }
                ));
    }

    return 0;
}

namespace details
{
namespace HostBootTime
{
static const auto _param_SetDuration =
        utility::tuple_to_array(message::types::type_id<
                std::string, uint64_t>());
static const auto _return_SetDuration =
        utility::tuple_to_array(message::types::type_id<
                sdbusplus::xyz::openbmc_project::Time::Boot::server::HostBootTime::SetDurationStates>());
}
}




namespace
{
/** String to enum mapping for HostBootTime::SetDurationStates */
static const std::tuple<const char*, HostBootTime::SetDurationStates> mappingHostBootTimeSetDurationStates[] =
        {
            std::make_tuple( "xyz.openbmc_project.Time.Boot.HostBootTime.SetDurationStates.KeyDurationSet",                 HostBootTime::SetDurationStates::KeyDurationSet ),
            std::make_tuple( "xyz.openbmc_project.Time.Boot.HostBootTime.SetDurationStates.ExtraDurationSet",                 HostBootTime::SetDurationStates::ExtraDurationSet ),
            std::make_tuple( "xyz.openbmc_project.Time.Boot.HostBootTime.SetDurationStates.DurationNotSettable",                 HostBootTime::SetDurationStates::DurationNotSettable ),
        };

} // anonymous namespace

auto HostBootTime::convertStringToSetDurationStates(const std::string& s) noexcept ->
        std::optional<SetDurationStates>
{
    auto i = std::find_if(
            std::begin(mappingHostBootTimeSetDurationStates),
            std::end(mappingHostBootTimeSetDurationStates),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mappingHostBootTimeSetDurationStates) == i)
    {
        return std::nullopt;
    }
    else
    {
        return std::get<1>(*i);
    }
}

auto HostBootTime::convertSetDurationStatesFromString(const std::string& s) ->
        SetDurationStates
{
    auto r = convertStringToSetDurationStates(s);

    if (!r)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return *r;
    }
}

std::string HostBootTime::convertSetDurationStatesToString(HostBootTime::SetDurationStates v)
{
    auto i = std::find_if(
            std::begin(mappingHostBootTimeSetDurationStates),
            std::end(mappingHostBootTimeSetDurationStates),
            [v](auto& e){ return v == std::get<1>(e); });
    if (i == std::end(mappingHostBootTimeSetDurationStates))
    {
        throw std::invalid_argument(std::to_string(static_cast<int>(v)));
    }
    return std::get<0>(*i);
}

const vtable_t HostBootTime::_vtable[] = {
    vtable::start(),

    vtable::method("Notify",
                   details::HostBootTime::_param_Notify
                        .data(),
                   details::HostBootTime::_return_Notify
                        .data(),
                   _callback_Notify),

    vtable::method("SetDuration",
                   details::HostBootTime::_param_SetDuration
                        .data(),
                   details::HostBootTime::_return_SetDuration
                        .data(),
                   _callback_SetDuration),
    vtable::end()
};

} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

