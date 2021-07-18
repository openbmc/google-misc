#pragma once

#include <sdbusplus/slot.hpp>
#include <stdplus/cancel.hpp>

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

struct BusCall : public stdplus::Cancelable
{
    sdbusplus::slot::slot slot;
    explicit BusCall(sdbusplus::slot::slot&& slot) : slot(std::move(slot))
    {}
    void cancel() noexcept override
    {
        delete this;
    }
};

/** @class Dbus
 *  @brief Overridable D-Bus interface for generic handler
 */
class Dbus
{
  public:
    /** An arbitrary timeout to ensure that clients don't linger forever */
    static constexpr auto asyncCallTimeout = std::chrono::minutes(3);

    using SubTreeMapping = std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>;

    virtual ~Dbus() = default;

    /** @brief Gets the D-Bus mapper information for all hoth instances
     *
     *  @throw exception::SdBusError - All dbus exceptions will be thrown
     *                                 with this exception
     *  @return The mapping of object paths and services to hoth interfaces.
     */
    virtual SubTreeMapping getHothdMapping() = 0;

    /** @brief Determines if a hothd instance is running on the system
     *
     *  @return True if the hoth is running, false for other errors.
     */
    virtual bool pingHothd(std::string_view hothId) = 0;
};

} // namespace internal

} // namespace ipmi_hoth
