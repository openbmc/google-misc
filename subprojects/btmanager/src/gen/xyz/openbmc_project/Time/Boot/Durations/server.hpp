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

class Durations
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
        Durations() = delete;
        Durations(const Durations&) = delete;
        Durations& operator=(const Durations&) = delete;
        Durations(Durations&&) = delete;
        Durations& operator=(Durations&&) = delete;
        virtual ~Durations() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Durations(bus_t& bus, const char* path);


        using PropertiesVariant = sdbusplus::utility::dedup_variant_t<
                std::vector<std::tuple<std::string, uint64_t>>,
                uint64_t>;

        /** @brief Constructor to initialize the object from a map of
         *         properties.
         *
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         *  @param[in] vals - Map of property name to value for initialization.
         */
        Durations(bus_t& bus, const char* path,
                     const std::map<std::string, PropertiesVariant>& vals,
                     bool skipSignal = false);



        /** Get value of OSUserspaceShutdown */
        virtual uint64_t osUserspaceShutdown() const;
        /** Set value of OSUserspaceShutdown with option to skip sending signal */
        virtual uint64_t osUserspaceShutdown(uint64_t value,
               bool skipSignal);
        /** Set value of OSUserspaceShutdown */
        virtual uint64_t osUserspaceShutdown(uint64_t value);
        /** Get value of OSKernelShutdown */
        virtual uint64_t osKernelShutdown() const;
        /** Set value of OSKernelShutdown with option to skip sending signal */
        virtual uint64_t osKernelShutdown(uint64_t value,
               bool skipSignal);
        /** Set value of OSKernelShutdown */
        virtual uint64_t osKernelShutdown(uint64_t value);
        /** Get value of BMCShutdown */
        virtual uint64_t bmcShutdown() const;
        /** Set value of BMCShutdown with option to skip sending signal */
        virtual uint64_t bmcShutdown(uint64_t value,
               bool skipSignal);
        /** Set value of BMCShutdown */
        virtual uint64_t bmcShutdown(uint64_t value);
        /** Get value of BMC */
        virtual uint64_t bmc() const;
        /** Set value of BMC with option to skip sending signal */
        virtual uint64_t bmc(uint64_t value,
               bool skipSignal);
        /** Set value of BMC */
        virtual uint64_t bmc(uint64_t value);
        /** Get value of BIOS */
        virtual uint64_t bios() const;
        /** Set value of BIOS with option to skip sending signal */
        virtual uint64_t bios(uint64_t value,
               bool skipSignal);
        /** Set value of BIOS */
        virtual uint64_t bios(uint64_t value);
        /** Get value of NerfKernel */
        virtual uint64_t nerfKernel() const;
        /** Set value of NerfKernel with option to skip sending signal */
        virtual uint64_t nerfKernel(uint64_t value,
               bool skipSignal);
        /** Set value of NerfKernel */
        virtual uint64_t nerfKernel(uint64_t value);
        /** Get value of NerfUserspace */
        virtual uint64_t nerfUserspace() const;
        /** Set value of NerfUserspace with option to skip sending signal */
        virtual uint64_t nerfUserspace(uint64_t value,
               bool skipSignal);
        /** Set value of NerfUserspace */
        virtual uint64_t nerfUserspace(uint64_t value);
        /** Get value of OSKernel */
        virtual uint64_t osKernel() const;
        /** Set value of OSKernel with option to skip sending signal */
        virtual uint64_t osKernel(uint64_t value,
               bool skipSignal);
        /** Set value of OSKernel */
        virtual uint64_t osKernel(uint64_t value);
        /** Get value of OSUserspace */
        virtual uint64_t osUserspace() const;
        /** Set value of OSUserspace with option to skip sending signal */
        virtual uint64_t osUserspace(uint64_t value,
               bool skipSignal);
        /** Set value of OSUserspace */
        virtual uint64_t osUserspace(uint64_t value);
        /** Get value of Unmeasured */
        virtual uint64_t unmeasured() const;
        /** Set value of Unmeasured with option to skip sending signal */
        virtual uint64_t unmeasured(uint64_t value,
               bool skipSignal);
        /** Set value of Unmeasured */
        virtual uint64_t unmeasured(uint64_t value);
        /** Get value of Extra */
        virtual std::vector<std::tuple<std::string, uint64_t>> extra() const;
        /** Set value of Extra with option to skip sending signal */
        virtual std::vector<std::tuple<std::string, uint64_t>> extra(std::vector<std::tuple<std::string, uint64_t>> value,
               bool skipSignal);
        /** Set value of Extra */
        virtual std::vector<std::tuple<std::string, uint64_t>> extra(std::vector<std::tuple<std::string, uint64_t>> value);

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


        /** @brief Emit interface added */
        void emit_added()
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _xyz_openbmc_project_Time_Boot_Durations_interface.emit_removed();
        }

        static constexpr auto interface = "xyz.openbmc_project.Time.Boot.Durations";

    private:

        /** @brief sd-bus callback for get-property 'OSUserspaceShutdown' */
        static int _callback_get_OSUserspaceShutdown(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'OSKernelShutdown' */
        static int _callback_get_OSKernelShutdown(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'BMCShutdown' */
        static int _callback_get_BMCShutdown(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'BMC' */
        static int _callback_get_BMC(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'BIOS' */
        static int _callback_get_BIOS(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'NerfKernel' */
        static int _callback_get_NerfKernel(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'NerfUserspace' */
        static int _callback_get_NerfUserspace(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'OSKernel' */
        static int _callback_get_OSKernel(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'OSUserspace' */
        static int _callback_get_OSUserspace(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'Unmeasured' */
        static int _callback_get_Unmeasured(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for get-property 'Extra' */
        static int _callback_get_Extra(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);


        static const vtable_t _vtable[];
        sdbusplus::server::interface_t
                _xyz_openbmc_project_Time_Boot_Durations_interface;
        sdbusplus::SdBusInterface *_intf;

        uint64_t _osUserspaceShutdown{};
        uint64_t _osKernelShutdown{};
        uint64_t _bmcShutdown{};
        uint64_t _bmc{};
        uint64_t _bios{};
        uint64_t _nerfKernel{};
        uint64_t _nerfUserspace{};
        uint64_t _osKernel{};
        uint64_t _osUserspace{};
        uint64_t _unmeasured{};
        std::vector<std::tuple<std::string, uint64_t>> _extra{};

};


} // namespace server
} // namespace Boot
} // namespace Time
} // namespace openbmc_project
} // namespace xyz

namespace message::details
{
} // namespace message::details
} // namespace sdbusplus

