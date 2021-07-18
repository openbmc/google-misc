// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
