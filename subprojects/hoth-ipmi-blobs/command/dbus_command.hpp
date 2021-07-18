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

#include <function2/function2.hpp>
#include <stdplus/cancel.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

/** @class DbusCommand
 *  @brief Overridable D-Bus interface for command handler
 */
class DbusCommand : public virtual Dbus
{
  public:
    using Cb = fu2::unique_function<void(std::optional<std::vector<uint8_t>>)>;

    /** @brief Implementation for asynchronous SendHostCommand
     *  Send a host command to Hoth and run a callback when it responds.
     *
     *  @param[in] hothId - The identifier of the targeted hoth instance.
     *  @param[in] command - Data to write to Hoth SPI host command offset.
     *  @param[in] cb      - The callback to execute.
     *  @return cancelable[stdplus::Cancel] - Drop to cancel the command early.
     */
    virtual stdplus::Cancel SendHostCommand(std::string_view hothId,
                                            const std::vector<uint8_t>&,
                                            Cb&&) = 0;
};

} // namespace internal

} // namespace ipmi_hoth
