/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <fmt/format.h>
#include <systemd/sd-bus.h>

#include <sdbusplus/message.hpp>

#include <stdexcept>

namespace kcsbridge
{

template <auto func, typename Data>
int methodRsp(sd_bus_message* mptr, void* dataptr, sd_bus_error* error) noexcept
{
    sdbusplus::message::message m(mptr);
    try
    {
        func(m, *reinterpret_cast<Data*>(dataptr));
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "Method response failed: {}\n", e.what());
        sd_bus_error_set(error,
                         "xyz.openbmc_project.Common.Error.InternalFailure",
                         "The operation failed internally.");
    }
    return 1;
}

} // namespace kcsbridge
