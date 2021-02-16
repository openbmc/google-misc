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

#include <fmt/format.h>
#include <getopt.h>
#include <linux/ipmi_bmc.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-daemon.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/handle/managed.hpp>
#include <stdplus/signal.hpp>

#include <stdexcept>
#include <type_traits>
#include <utility>

using sdeventplus::source::IO;
using sdeventplus::source::Signal;
using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlag;
using stdplus::fd::OpenFlags;

struct Opts
{
    size_t log_level = 0;
    const char* channel = nullptr;
};

Opts parse(int argc, char* argv[])
{
    static const char opts[] = ":c:v";
    static const struct option longopts[] = {
        {"channel", required_argument, nullptr, 'c'},
        {"verbose", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0},
    };
    int c;
    optind = 0;
    Opts ret;
    while ((c = getopt_long(argc, argv, opts, longopts, nullptr)) > 0)
    {
        switch (c)
        {
            case 'c':
                ret.channel = optarg;
                break;
            case 'v':
                reinterpret_cast<uint8_t&>(ret.log_level)++;
                break;
            case ':':
                throw std::runtime_error(
                    fmt::format("Missing argument for `{}`", argv[optind - 1]));
                break;
            default:
                throw std::runtime_error(fmt::format(
                    "Invalid command line argument `{}`", argv[optind - 1]));
        }
    }
    if (optind != argc)
    {
        throw std::invalid_argument("Requires no additional arguments");
    }
    if (ret.channel == nullptr)
    {
        throw std::invalid_argument("Missing KCS channel");
    }
    return ret;
}

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
    auto ioCb = [&](IO&, int, uint32_t) {
        try
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
                throw std::invalid_argument("Read too small");
            }
            auto m = bus.new_method_call(
                "xyz.openbmc_project.Ipmi.Host", "/xyz/openbmc_project/Ipmi",
                "xyz.openbmc_project.Ipmi.Server", "execute");
            std::map<std::string, std::variant<int>> options;
            uint8_t netfn = in[0] >> 2, lun = in[0] & 3;
            m.append(netfn, lun, in[1], in.subspan(2), options);
            slot = busCallAsync(
                std::move(m), [&](sdbusplus::message::message&& m) {
                    slot.reset();
                    stdplus::span<uint8_t> out(buffer.begin(), 3);
                    try
                    {
                        std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,
                                   std::vector<uint8_t>>
                            ret;
                        m.read(ret);
                        buffer[0] = (std::get<0>(ret) | 1) << 2;
                        buffer[0] |= std::get<1>(ret);
                        buffer[1] = std::get<2>(ret);
                        buffer[2] = std::get<3>(ret);
                        auto& data = std::get<4>(ret);
                        memcpy(&buffer[3], data.data(), data.size());
                        out = stdplus::span<uint8_t>(buffer.begin(),
                                                     data.size() + 3);
                    }
                    catch (const std::exception& e)
                    {
                        fmt::print(stderr, "IPMI response failure: {}\n",
                                   e.what());
                        buffer[0] |= 1 << 2;
                        buffer[2] = 0xff;
                    }

                    stdplus::fd::writeExact(kcs, out);
                });
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Failed reading: {}\n", e.what());
        }
    };
    IO ioSource(event, kcs.get(), EPOLLIN | EPOLLET, std::move(ioCb));

    sd_notify(0, "READY=1");
    return event.loop();
}

int main(int argc, char* argv[])
{
    try
    {
        auto opts = parse(argc, argv);
        return execute(opts.channel);
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "FAILED: {}\n", e.what());
        return 1;
    }
}
