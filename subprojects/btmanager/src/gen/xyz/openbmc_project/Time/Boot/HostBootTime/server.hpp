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

class HostBootTime
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
        HostBootTime() = delete;
        HostBootTime(const HostBootTime&) = delete;
        HostBootTime& operator=(const HostBootTime&) = delete;
        HostBootTime(HostBootTime&&) = delete;
        HostBootTime& operator=(HostBootTime&&) = delete;
        virtual ~HostBootTime() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        HostBootTime(bus_t& bus, const char* path);

        enum class SetDurationStates
        {
            KeyDurationSet,
            ExtraDurationSet,
            DurationNotSettable,
        };


        /** @brief Implementation for Notify
         *  Notifies BMC to label current monotonic time as <Timepoint> of a stage.
         *
         *  @param[in] timepoint - Current timepoint code.
         *
         *  @return timestamp[uint64_t] - The timestamp that BMC give to this stage.
         */
        virtual uint64_t notify(
            uint8_t timepoint) = 0;

        /** @brief Implementation for SetDuration
         *  Directly set duration to a stage.
         *
         *  @param[in] stage - Stage name.
         *  @param[in] durationMicrosecond - Duration in microsecond of this stage.
         *
         *  @return state[SetDurationStates] - The timestamp that BMC give to this stage.
         */
        virtual SetDurationStates setDuration(
            std::string stage,
            uint64_t durationMicrosecond) = 0;



        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "xyz.openbmc_project.Time.Boot.HostBootTime.<value name>"
         *  @return - The enum value.
         *
         *  @note Throws if string is not a valid mapping.
         */
        static SetDurationStates convertSetDurationStatesFromString(const std::string& s);

        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "xyz.openbmc_project.Time.Boot.HostBootTime.<value name>"
         *  @return - The enum value or std::nullopt
         */
        static std::optional<SetDurationStates> convertStringToSetDurationStates(
                const std::string& s) noexcept;

        /** @brief Convert an enum value to a string.
         *  @param[in] e - The enum to convert to a string.
         *  @return - The string conversion in the form of
         *            "xyz.openbmc_project.Time.Boot.HostBootTime.<value name>"
         */
        static std::string convertSetDurationStatesToString(SetDurationStates e);

        /** @brief Emit interface added */
        void emit_added()
        {
            _xyz_openbmc_project_Time_Boot_HostBootTime_interface.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _xyz_openbmc_project_Time_Boot_HostBootTime_interface.emit_removed();
        }

        static constexpr auto interface = "xyz.openbmc_project.Time.Boot.HostBootTime";

    private:

        /** @brief sd-bus callback for Notify
         */
        static int _callback_Notify(
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for SetDuration
         */
        static int _callback_SetDuration(
            sd_bus_message*, void*, sd_bus_error*);


        static const vtable_t _vtable[];
        sdbusplus::server::interface_t
                _xyz_openbmc_project_Time_Boot_HostBootTime_interface;
        sdbusplus::SdBusInterface *_intf;


};

/* Specialization of sdbusplus::server::convertForMessage
 * for enum-type HostBootTime::SetDurationStates.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
inline std::string convertForMessage(HostBootTime::SetDurationStates e)
{
    return HostBootTime::convertSetDurationStatesToString(e);
}

} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz

namespace message::details
{
template <>
struct convert_from_string<xyz::openbmc_project::Time::Boot::server::HostBootTime::SetDurationStates>
{
    static auto op(const std::string& value) noexcept
    {
        return xyz::openbmc_project::Time::Boot::server::HostBootTime::convertStringToSetDurationStates(value);
    }
};

template <>
struct convert_to_string<xyz::openbmc_project::Time::Boot::server::HostBootTime::SetDurationStates>
{
    static std::string op(xyz::openbmc_project::Time::Boot::server::HostBootTime::SetDurationStates value)
    {
        return xyz::openbmc_project::Time::Boot::server::HostBootTime::convertSetDurationStatesToString(value);
    }
};
} // namespace message::details
} // namespace sdbusplus

