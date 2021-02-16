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
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/signal.hpp>
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
using sdeventplus::source::IO;
using sdeventplus::source::Signal;
using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlag;
using stdplus::fd::OpenFlags;

void write(stdplus::Fd& kcs, message&& m)
{
    std::array<uint8_t, 1024> buffer;
    stdplus::span<uint8_t> out(buffer.begin(), 3);
    try
    {
        std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
            ret;
        m.read(ret);
        buffer[0] = (std::get<0>(ret) | 1) << 2;
        buffer[0] |= std::get<1>(ret);
        buffer[1] = std::get<2>(ret);
        buffer[2] = std::get<3>(ret);
        auto& data = std::get<4>(ret);
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

void read(stdplus::Fd& kcs, bus& bus, ManagedSdBusSlot& slot)
{
    std::array<uint8_t, 1024> buffer;
    auto in = stdplus::fd::read(kcs, buffer);
    if (in.empty())
    {
        return;
    }
    if (slot)
    {
        fmt::print(stderr, "Canceling outstanding request\n");
        slot.reset();
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
    uint8_t netfn = in[0] >> 2, lun = in[0] & 3;
    m.append(netfn, lun, in[1], in.subspan(2), options);
    slot = busCallAsync(std::move(m), [&](message&& m) {
        slot.reset();
        write(kcs, std::move(m));
    });
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
    ManagedSdBusSlot slot(std::nullopt);

    // Add a reader to the bus for handling inbound IPMI
    IO ioSource(event, kcs.get(), EPOLLIN | EPOLLET,
                [&](IO&, int, uint32_t) { read(kcs, bus, slot); });

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
