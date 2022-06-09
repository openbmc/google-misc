#include <algorithm>
#include <map>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <xyz/openbmc_project/Time/Boot/Durations/server.hpp>












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

Durations::Durations(bus_t& bus, const char* path)
        : _xyz_openbmc_project_Time_Boot_Durations_interface(
                bus, path, interface, _vtable, this), _intf(bus.getInterface())
{
}

Durations::Durations(bus_t& bus, const char* path,
                           const std::map<std::string, PropertiesVariant>& vals,
                           bool skipSignal)
        : Durations(bus, path)
{
    for (const auto& v : vals)
    {
        setPropertyByName(v.first, v.second, skipSignal);
    }
}




auto Durations::osUserspaceShutdown() const ->
        uint64_t
{
    return _osUserspaceShutdown;
}

int Durations::_callback_get_OSUserspaceShutdown(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->osUserspaceShutdown();
                    }
                ));
    }
}

auto Durations::osUserspaceShutdown(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_osUserspaceShutdown != value)
    {
        _osUserspaceShutdown = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("OSUserspaceShutdown");
        }
    }

    return _osUserspaceShutdown;
}

auto Durations::osUserspaceShutdown(uint64_t val) ->
        uint64_t
{
    return osUserspaceShutdown(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_OSUserspaceShutdown =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::osKernelShutdown() const ->
        uint64_t
{
    return _osKernelShutdown;
}

int Durations::_callback_get_OSKernelShutdown(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->osKernelShutdown();
                    }
                ));
    }
}

auto Durations::osKernelShutdown(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_osKernelShutdown != value)
    {
        _osKernelShutdown = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("OSKernelShutdown");
        }
    }

    return _osKernelShutdown;
}

auto Durations::osKernelShutdown(uint64_t val) ->
        uint64_t
{
    return osKernelShutdown(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_OSKernelShutdown =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::bmcShutdown() const ->
        uint64_t
{
    return _bmcShutdown;
}

int Durations::_callback_get_BMCShutdown(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->bmcShutdown();
                    }
                ));
    }
}

auto Durations::bmcShutdown(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_bmcShutdown != value)
    {
        _bmcShutdown = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("BMCShutdown");
        }
    }

    return _bmcShutdown;
}

auto Durations::bmcShutdown(uint64_t val) ->
        uint64_t
{
    return bmcShutdown(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_BMCShutdown =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::bmc() const ->
        uint64_t
{
    return _bmc;
}

int Durations::_callback_get_BMC(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->bmc();
                    }
                ));
    }
}

auto Durations::bmc(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_bmc != value)
    {
        _bmc = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("BMC");
        }
    }

    return _bmc;
}

auto Durations::bmc(uint64_t val) ->
        uint64_t
{
    return bmc(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_BMC =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::bios() const ->
        uint64_t
{
    return _bios;
}

int Durations::_callback_get_BIOS(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->bios();
                    }
                ));
    }
}

auto Durations::bios(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_bios != value)
    {
        _bios = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("BIOS");
        }
    }

    return _bios;
}

auto Durations::bios(uint64_t val) ->
        uint64_t
{
    return bios(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_BIOS =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::nerfKernel() const ->
        uint64_t
{
    return _nerfKernel;
}

int Durations::_callback_get_NerfKernel(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->nerfKernel();
                    }
                ));
    }
}

auto Durations::nerfKernel(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_nerfKernel != value)
    {
        _nerfKernel = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("NerfKernel");
        }
    }

    return _nerfKernel;
}

auto Durations::nerfKernel(uint64_t val) ->
        uint64_t
{
    return nerfKernel(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_NerfKernel =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::nerfUserspace() const ->
        uint64_t
{
    return _nerfUserspace;
}

int Durations::_callback_get_NerfUserspace(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->nerfUserspace();
                    }
                ));
    }
}

auto Durations::nerfUserspace(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_nerfUserspace != value)
    {
        _nerfUserspace = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("NerfUserspace");
        }
    }

    return _nerfUserspace;
}

auto Durations::nerfUserspace(uint64_t val) ->
        uint64_t
{
    return nerfUserspace(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_NerfUserspace =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::osKernel() const ->
        uint64_t
{
    return _osKernel;
}

int Durations::_callback_get_OSKernel(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->osKernel();
                    }
                ));
    }
}

auto Durations::osKernel(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_osKernel != value)
    {
        _osKernel = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("OSKernel");
        }
    }

    return _osKernel;
}

auto Durations::osKernel(uint64_t val) ->
        uint64_t
{
    return osKernel(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_OSKernel =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::osUserspace() const ->
        uint64_t
{
    return _osUserspace;
}

int Durations::_callback_get_OSUserspace(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->osUserspace();
                    }
                ));
    }
}

auto Durations::osUserspace(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_osUserspace != value)
    {
        _osUserspace = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("OSUserspace");
        }
    }

    return _osUserspace;
}

auto Durations::osUserspace(uint64_t val) ->
        uint64_t
{
    return osUserspace(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_OSUserspace =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::unmeasured() const ->
        uint64_t
{
    return _unmeasured;
}

int Durations::_callback_get_Unmeasured(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->unmeasured();
                    }
                ));
    }
}

auto Durations::unmeasured(uint64_t value,
                                         bool skipSignal) ->
        uint64_t
{
    if (_unmeasured != value)
    {
        _unmeasured = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("Unmeasured");
        }
    }

    return _unmeasured;
}

auto Durations::unmeasured(uint64_t val) ->
        uint64_t
{
    return unmeasured(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_Unmeasured =
    utility::tuple_to_array(message::types::type_id<
            uint64_t>());
}
}

auto Durations::extra() const ->
        std::vector<std::tuple<std::string, uint64_t>>
{
    return _extra;
}

int Durations::_callback_get_Extra(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<Durations*>(context);

    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->extra();
                    }
                ));
    }
}

auto Durations::extra(std::vector<std::tuple<std::string, uint64_t>> value,
                                         bool skipSignal) ->
        std::vector<std::tuple<std::string, uint64_t>>
{
    if (_extra != value)
    {
        _extra = value;
        if (!skipSignal)
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.property_changed("Extra");
        }
    }

    return _extra;
}

auto Durations::extra(std::vector<std::tuple<std::string, uint64_t>> val) ->
        std::vector<std::tuple<std::string, uint64_t>>
{
    return extra(val, false);
}


namespace details
{
namespace Durations
{
static const auto _property_Extra =
    utility::tuple_to_array(message::types::type_id<
            std::vector<std::tuple<std::string, uint64_t>>>());
}
}

void Durations::setPropertyByName(const std::string& _name,
                                     const PropertiesVariant& val,
                                     bool skipSignal)
{
    if (_name == "OSUserspaceShutdown")
    {
        auto& v = std::get<uint64_t>(val);
        osUserspaceShutdown(v, skipSignal);
        return;
    }
    if (_name == "OSKernelShutdown")
    {
        auto& v = std::get<uint64_t>(val);
        osKernelShutdown(v, skipSignal);
        return;
    }
    if (_name == "BMCShutdown")
    {
        auto& v = std::get<uint64_t>(val);
        bmcShutdown(v, skipSignal);
        return;
    }
    if (_name == "BMC")
    {
        auto& v = std::get<uint64_t>(val);
        bmc(v, skipSignal);
        return;
    }
    if (_name == "BIOS")
    {
        auto& v = std::get<uint64_t>(val);
        bios(v, skipSignal);
        return;
    }
    if (_name == "NerfKernel")
    {
        auto& v = std::get<uint64_t>(val);
        nerfKernel(v, skipSignal);
        return;
    }
    if (_name == "NerfUserspace")
    {
        auto& v = std::get<uint64_t>(val);
        nerfUserspace(v, skipSignal);
        return;
    }
    if (_name == "OSKernel")
    {
        auto& v = std::get<uint64_t>(val);
        osKernel(v, skipSignal);
        return;
    }
    if (_name == "OSUserspace")
    {
        auto& v = std::get<uint64_t>(val);
        osUserspace(v, skipSignal);
        return;
    }
    if (_name == "Unmeasured")
    {
        auto& v = std::get<uint64_t>(val);
        unmeasured(v, skipSignal);
        return;
    }
    if (_name == "Extra")
    {
        auto& v = std::get<std::vector<std::tuple<std::string, uint64_t>>>(val);
        extra(v, skipSignal);
        return;
    }
}

auto Durations::getPropertyByName(const std::string& _name) ->
        PropertiesVariant
{
    if (_name == "OSUserspaceShutdown")
    {
        return osUserspaceShutdown();
    }
    if (_name == "OSKernelShutdown")
    {
        return osKernelShutdown();
    }
    if (_name == "BMCShutdown")
    {
        return bmcShutdown();
    }
    if (_name == "BMC")
    {
        return bmc();
    }
    if (_name == "BIOS")
    {
        return bios();
    }
    if (_name == "NerfKernel")
    {
        return nerfKernel();
    }
    if (_name == "NerfUserspace")
    {
        return nerfUserspace();
    }
    if (_name == "OSKernel")
    {
        return osKernel();
    }
    if (_name == "OSUserspace")
    {
        return osUserspace();
    }
    if (_name == "Unmeasured")
    {
        return unmeasured();
    }
    if (_name == "Extra")
    {
        return extra();
    }

    return PropertiesVariant();
}


const vtable_t Durations::_vtable[] = {
    vtable::start(),
    vtable::property("OSUserspaceShutdown",
                     details::Durations::_property_OSUserspaceShutdown
                        .data(),
                     _callback_get_OSUserspaceShutdown,
                     vtable::property_::const_),
    vtable::property("OSKernelShutdown",
                     details::Durations::_property_OSKernelShutdown
                        .data(),
                     _callback_get_OSKernelShutdown,
                     vtable::property_::const_),
    vtable::property("BMCShutdown",
                     details::Durations::_property_BMCShutdown
                        .data(),
                     _callback_get_BMCShutdown,
                     vtable::property_::const_),
    vtable::property("BMC",
                     details::Durations::_property_BMC
                        .data(),
                     _callback_get_BMC,
                     vtable::property_::const_),
    vtable::property("BIOS",
                     details::Durations::_property_BIOS
                        .data(),
                     _callback_get_BIOS,
                     vtable::property_::const_),
    vtable::property("NerfKernel",
                     details::Durations::_property_NerfKernel
                        .data(),
                     _callback_get_NerfKernel,
                     vtable::property_::const_),
    vtable::property("NerfUserspace",
                     details::Durations::_property_NerfUserspace
                        .data(),
                     _callback_get_NerfUserspace,
                     vtable::property_::const_),
    vtable::property("OSKernel",
                     details::Durations::_property_OSKernel
                        .data(),
                     _callback_get_OSKernel,
                     vtable::property_::const_),
    vtable::property("OSUserspace",
                     details::Durations::_property_OSUserspace
                        .data(),
                     _callback_get_OSUserspace,
                     vtable::property_::const_),
    vtable::property("Unmeasured",
                     details::Durations::_property_Unmeasured
                        .data(),
                     _callback_get_Unmeasured,
                     vtable::property_::const_),
    vtable::property("Extra",
                     details::Durations::_property_Extra
                        .data(),
                     _callback_get_Extra,
                     vtable::property_::const_),
    vtable::end()
};

} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

