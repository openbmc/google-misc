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
#include <stdplus/handle/managed.hpp>
#include <stdplus/util/cexec.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace kcsbridge
{

struct SdBusSlotDrop
{
    void operator()(sd_bus_slot* slot) noexcept
    {
        sd_bus_slot_unref(slot);
    }
};
using ManagedSdBusSlot = stdplus::Managed<sd_bus_slot*>::HandleF<SdBusSlotDrop>;

template <typename CbT>
int busCallAsyncCb(sd_bus_message* m, void* userdata, sd_bus_error*) noexcept
{
    try
    {
        (*reinterpret_cast<CbT*>(userdata))(sdbusplus::message::message(m));
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "Callback failed: {}\n", e.what());
    }
    return 1;
}

template <typename CbT>
void busCallAsyncDest(void* userdata) noexcept
{
    delete reinterpret_cast<CbT*>(userdata);
}

template <typename Cb>
auto busCallAsync(sdbusplus::message::message&& m, Cb&& cb)
{
    sd_bus_slot* slot;
    using CbT = std::remove_cv_t<std::remove_reference_t<Cb>>;
    CHECK_RET(sd_bus_call_async(nullptr, &slot, m.get(), busCallAsyncCb<CbT>,
                                nullptr, std::numeric_limits<uint64_t>::max()),
              "sd_bus_call_async");
    ManagedSdBusSlot ret(std::move(slot));
    CHECK_RET(sd_bus_slot_set_destroy_callback(*ret, busCallAsyncDest<CbT>),
              "sd_bus_slot_set_destroy_callback");
    sd_bus_slot_set_userdata(*ret, new CbT(std::forward<Cb>(cb)));
    return ret;
}

template <auto fun, typename Data>
int methodRsp(sd_bus_message* mptr, void* dataptr, sd_bus_error* error) noexcept
{
    sdbusplus::message::message m(mptr);
    try
    {
        fun(m, *reinterpret_cast<Data*>(dataptr));
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
