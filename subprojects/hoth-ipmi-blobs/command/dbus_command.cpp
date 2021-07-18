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

#include "dbus_command.hpp"

#include "dbus_command_impl.hpp"

#include <fmt/format.h>

#include <stdplus/cancel.hpp>

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

stdplus::Cancel
    DbusCommandImpl::SendHostCommand(std::string_view hothId,
                                     const std::vector<uint8_t>& cmd, Cb&& cb)
{
    auto req = newHothdCall(hothId, "SendHostCommand");
    req.append(cmd);
    auto cHothId = std::string(hothId);
    return stdplus::Cancel(new BusCall(req.call_async(
        [cb = std::move(cb), hothId = std::move(cHothId)](
            sdbusplus::message::message m) mutable noexcept {
            if (m.is_method_error())
            {
                auto err = m.get_error();
                fmt::print(stderr, "SendHostCommand failed on `{}`: {}: {}\n",
                           hothId, err->name, err->message);
                cb(std::nullopt);
                return;
            }
            try
            {
                std::vector<uint8_t> rsp;
                m.read(rsp);
                cb(std::move(rsp));
            }
            catch (const std::exception& e)
            {
                fmt::print(stderr,
                           "SendHostCommand failed unpacking on `{}`: {}\n",
                           hothId, e.what());
                cb(std::nullopt);
            }
        },
        asyncCallTimeout)));
}

} // namespace internal

} // namespace ipmi_hoth
