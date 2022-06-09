#pragma once
#include <limits>
#include <map>
#include <optional>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>
#include <string>
#include <systemd/sd-bus.h>
#include <tuple>



#ifndef SDBUSPP_NEW_CAMELCASE
#define SDBUSPP_NEW_CAMELCASE 1
#endif

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

class Statistic
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
        Statistic() = delete;
        Statistic(const Statistic&) = delete;
        Statistic& operator=(const Statistic&) = delete;
        Statistic(Statistic&&) = delete;
        Statistic& operator=(Statistic&&) = delete;
        virtual ~Statistic() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Statistic(bus_t& bus, const char* path);

        enum class PowerCycleType
        {
            ACPowerCycle,
            DCPowerCycle,
        };

        using PropertiesVariant = sdbusplus::utility::dedup_variant_t<
                PowerCycleType,
                uint32_t>;

        /** @brief Constructor to initialize the object from a map of
         *         properties.
         *
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         *  @param[in] vals - Map of property name to value for initialization.
         */
        Statistic(bus_t& bus, const char* path,
                     const std::map<std::string, PropertiesVariant>& vals,
                     bool skipSignal = false);



        /** Get value of InternalRebootCount */
        virtual uint32_t internalRebootCount() const;
        /** Set value of InternalRebootCount with option to skip sending signal */
        virtual uint32_t internalRebootCount(uint32_t value,
               bool skipSignal);
        /** Set value of InternalRebootCount */
        virtual uint32_t internalRebootCount(uint32_t value);
        /** Get value of PowerCycleType */
        virtual PowerCycleType powerCycleType() const;
        /** Set value of PowerCycleType with option to skip sending signal */
        virtual PowerCycleType powerCycleType(PowerCycleType value,
               bool skipSignal);
        /** Set value of PowerCycleType */
        virtual PowerCycleType powerCycleType(PowerCycleType value);

        /** @brief Sets a property by name.
         *  @param[in] _name - A string representation of the property name.
         *  @param[in] val - A variant containing the value to set.
         */
        void setPropertyByName(const std::string& _name,
                               const PropertiesVariant& val,
                               bool skipSignal = false);

        /** @brief Gets a property by name.
         *  @param[in] _name - A string representation of the property name.
         *  @return - A variant containing the value of the property.
         */
        PropertiesVariant getPropertyByName(const std::string& _name);

        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "xyz.openbmc_project.Time.Boot.Statistic.<value name>"
         *  @return - The enum value.
         *
         *  @note Throws if string is not a valid mapping.
         */
        static PowerCycleType convertPowerCycleTypeFromString(const std::string& s);

        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "xyz.openbmc_project.Time.Boot.Statistic.<value name>"
         *  @return - The enum value or std::nullopt
         */
        static std::optional<PowerCycleType> convertStringToPowerCycleType(
                const std::string& s) noexcept;

        /** @brief Convert an enum value to a string.
         *  @param[in] e - The enum to convert to a string.
         *  @return - The string conversion in the form of
         *            "xyz.openbmc_project.Time.Boot.Statistic.<value name>"
         */
        static std::string convertPowerCycleTypeToString(PowerCycleType e);

        /** @brief Emit interface added */
        void emit_added()
        {
            _xyz_openbmc_project_Time_Boot_Statistic_interface.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _xyz_openbmc_project_Time_Boot_Statistic_interface.emit_removed();
        }

        static constexpr auto interface = "xyz.openbmc_project.Time.Boot.Statistic";

    private:

        /** @brief sd-bus callback for get-property 'InternalRebootCount' */
        static int _callback_get_InternalRebootCount(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'InternalRebootCount' */
        static int _callback_set_InternalRebootCount(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'PowerCycleType' */
        static int _callback_get_PowerCycleType(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property 'PowerCycleType' */
        static int _callback_set_PowerCycleType(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);


        static const vtable_t _vtable[];
        sdbusplus::server::interface_t
                _xyz_openbmc_project_Time_Boot_Statistic_interface;
        sdbusplus::SdBusInterface *_intf;

        uint32_t _internalRebootCount = 0;
        PowerCycleType _powerCycleType{};

};

/* Specialization of sdbusplus::server::convertForMessage
 * for enum-type Statistic::PowerCycleType.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
inline std::string convertForMessage(Statistic::PowerCycleType e)
{
    return Statistic::convertPowerCycleTypeToString(e);
}

} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz

namespace message::details
{
template <>
struct convert_from_string<xyz::openbmc_project::Time::Boot::server::Statistic::PowerCycleType>
{
    static auto op(const std::string& value) noexcept
    {
        return xyz::openbmc_project::Time::Boot::server::Statistic::convertStringToPowerCycleType(value);
    }
};

template <>
struct convert_to_string<xyz::openbmc_project::Time::Boot::server::Statistic::PowerCycleType>
{
    static std::string op(xyz::openbmc_project::Time::Boot::server::Statistic::PowerCycleType value)
    {
        return xyz::openbmc_project::Time::Boot::server::Statistic::convertPowerCycleTypeToString(value);
    }
};
} // namespace message::details
} // namespace sdbusplus

