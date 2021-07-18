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

#include "dbus.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <string_view>

namespace ipmi_hoth
{
namespace internal
{

/** @class DbusImpl
 *  @brief D-Bus concrete implementation
 *  @details Pass through all calls to the default D-Bus instance
 */
class DbusImpl : public virtual Dbus
{
  public:
    /** @brief Timeout suitable for responding to IPMI queries before
     *         the sending mechanism like kcsbridge issues a retry.
     */
    static constexpr auto kTimeout = std::chrono::seconds(4);

    DbusImpl();

    SubTreeMapping getHothdMapping() override;
    bool pingHothd(std::string_view hothId) override;

  protected:
    /** @brief Helper for building a message targeting a hoth daemon
     *
     *  @param[in] hothId - The identifier of the hoth daemon
     *  @param[in] intf    - The name of the D-Bus interface being called
     *  @param[in] method  - The D-Bus method being called on the interface
     *  @return The empty message which can be sent.
     */
    sdbusplus::message::message newHothdCall(std::string_view hothId,
                                             const char* intf,
                                             const char* method);

    /** @brief Helper for building a message targeting a hoth daemon
     *         assuming the call is for the hoth interface.
     *
     *  @param[in] hothId - The identifier of the hoth daemon
     *  @param[in] method  - The D-Bus method being called on the interface
     *  @return The empty message which can be sent.
     */
    sdbusplus::message::message newHothdCall(std::string_view hothId,
                                             const char* method);

    sdbusplus::bus::bus bus;
};

} // namespace internal

} // namespace ipmi_hoth
