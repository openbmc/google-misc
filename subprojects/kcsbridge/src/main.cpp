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

#include "args.hpp"
#include "dbus.hpp"

#include <fmt/format.h>
#include <linux/ipmi_bmc.h>
#include <systemd/sd-daemon.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/slot.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/exception.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/signal.hpp>

#include <array>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <variant>

namespace kcsbridge
{

using sdbusplus::bus::bus;
using sdbusplus::message::message;
using sdbusplus::slot::slot;
using sdeventplus::source::IO;
using sdeventplus::source::Signal;
using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlag;
using stdplus::fd::OpenFlags;

void setAttention(message& m, stdplus::Fd& kcs)
{
    stdplus::fd::ioctl(kcs, IPMI_BMC_IOCTL_SET_SMS_ATN, nullptr);
    m.new_method_return().method_return();
}

void clearAttention(message& m, stdplus::Fd& kcs)
{
    stdplus::fd::ioctl(kcs, IPMI_BMC_IOCTL_CLEAR_SMS_ATN, nullptr);
    m.new_method_return().method_return();
}

void forceAbort(message& m, stdplus::Fd& kcs)
{
    stdplus::fd::ioctl(kcs, IPMI_BMC_IOCTL_FORCE_ABORT, nullptr);
    m.new_method_return().method_return();
}

template <typename Data>
constexpr sdbusplus::vtable::vtable_t dbusMethods[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method("setAttention", "", "",
                              methodRsp<setAttention, Data>),
    sdbusplus::vtable::method("clearAttention", "", "",
                              methodRsp<clearAttention, Data>),
    sdbusplus::vtable::method("forceAbort", "", "",
                              methodRsp<forceAbort, Data>),
    sdbusplus::vtable::end(),
};

void write(stdplus::Fd& kcs, message&& m)
{
    std::array<uint8_t, 1024> buffer;
    stdplus::span<uint8_t> out(buffer.begin(), 3);
    try
    {
        if (m.is_method_error())
        {
            // Extra copy to workaround lack of `const sd_bus_error` constructor
            auto error = *m.get_error();
            throw sdbusplus::exception::SdBusError(&error, "ipmid response");
        }
        std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
            ret;
        m.read(ret);
        const auto& [netfn, lun, cmd, cc, data] = ret;
        // Based on the IPMI KCS spec Figure 9-2
        // netfn needs to be changed to odd in KCS responses
        buffer[0] = (netfn | 1) << 2;
        buffer[0] |= lun;
        buffer[1] = cmd;
        buffer[2] = cc;
        memcpy(&buffer[3], data.data(), data.size());
        out = stdplus::span<uint8_t>(buffer.begin(), data.size() + 3);
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "IPMI response failure: {}\n", e.what());
        buffer[0] |= 1 << 2;
        buffer[2] = 0xff;
    }
    stdplus::fd::writeExact(kcs, out);
}

void read(stdplus::Fd& kcs, bus& bus, sdbusplus::slot::slot& outstanding)
{
    std::array<uint8_t, 1024> buffer;
    auto in = stdplus::fd::read(kcs, buffer);
    if (in.empty())
    {
        return;
    }
    if (outstanding)
    {
        fmt::print(stderr, "Canceling outstanding request\n");
        outstanding = slot(nullptr);
    }
    if (in.size() < 2)
    {
        fmt::print(stderr, "Read too small, ignoring\n");
        return;
    }
    auto m = bus.new_method_call("xyz.openbmc_project.Ipmi.Host",
                                 "/xyz/openbmc_project/Ipmi",
                                 "xyz.openbmc_project.Ipmi.Server", "execute");
    std::map<std::string, std::variant<int>> options;
    // Based on the IPMI KCS spec Figure 9-1
    uint8_t netfn = in[0] >> 2, lun = in[0] & 3, cmd = in[1];
    m.append(netfn, lun, cmd, in.subspan(2), options);
    outstanding = m.call_async(stdplus::exception::ignore([&](message&& m) {
        outstanding = slot(nullptr);
        write(kcs, std::move(m));
    }));
}

int execute(const char* channel)
{
    // Set up our DBus and event loop
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // Configure basic signal handling
    auto exit_handler = [&](Signal&, const struct signalfd_siginfo*) {
        fmt::print(stderr, "Interrupted, Exiting\n");
        event.exit(0);
    };
    stdplus::signal::block(SIGINT);
    Signal sig_int(event, SIGINT, exit_handler);
    stdplus::signal::block(SIGTERM);
    Signal sig_term(event, SIGTERM, exit_handler);

    // Open an FD for the KCS channel
    stdplus::ManagedFd kcs = stdplus::fd::open(
        fmt::format("/dev/{}", channel),
        OpenFlags(OpenAccess::ReadWrite).set(OpenFlag::NonBlock));
    sdbusplus::slot::slot slot(nullptr);

    // Add a reader to the bus for handling inbound IPMI
    IO ioSource(event, kcs.get(), EPOLLIN | EPOLLET,
                stdplus::exception::ignore(
                    [&](IO&, int, uint32_t) { read(kcs, bus, slot); }));

    // Allow processes to affect the state machine
    std::optional<sdbusplus::server::interface::interface> intf;
    {
        std::string dbusChannel = channel;
        std::replace(dbusChannel.begin(), dbusChannel.end(), '-', '_');
        auto obj = "/xyz/openbmc_project/Ipmi/Channel/" + dbusChannel;
        auto srv = "com.google.gbmc." + dbusChannel;
        intf.emplace(bus, obj.c_str(), "xyz.openbmc_project.Ipmi.Channel.SMS",
                     dbusMethods<stdplus::Fd>,
                     reinterpret_cast<stdplus::Fd*>(&kcs));
        bus.request_name(srv.c_str());
    }

    sd_notify(0, "READY=1");
    return event.loop();
}

} // namespace kcsbridge

int main(int argc, char* argv[])
{
    try
    {
        kcsbridge::Args args(argc, argv);
        return kcsbridge::execute(args.channel);
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "FAILED: {}\n", e.what());
        return 1;
    }
}
