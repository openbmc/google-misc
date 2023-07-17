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

#include "src/default_addresses.h"

#include "src/host_manager.hpp"
#include "src/nemora.hpp"

#include <arpa/inet.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>

#include <csignal>
#include <cstdint>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using phosphor::logging::level;
using phosphor::logging::log;

namespace
{
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
    gSignalStatus = signal;
}

void NemoraUdpPoll(Nemora* nemora)
{
    while (!gSignalStatus)
    {
        nemora->UdpPoll();
    }
}

int main(int argc, char* argv[])
{
    // Init arg parser
    CLI::App app("gBMC-side Nemora implementation (POST-code only)");

    std::string udp_address_v4_str;
    auto* ipv4_option = app.add_option("--udp4", udp_address_v4_str,
                                       "Target IPv4 address for UDP "
                                       "communication, i.e., POST streaming.")
                            ->capture_default_str();

    std::string udp_address_v6_str;
    auto* ipv6_option = app.add_option("--udp6", udp_address_v6_str,
                                       "Target IPv6 address for UDP "
                                       "communication, i.e., POST streaming.")
                            ->capture_default_str();

    // interface is last, and required.
    std::string iface_name;
    app.add_option("interface", iface_name,
                   "Network interface for TCP communication. Ex: eth0")
        ->required();

    CLI11_PARSE(app, argc, argv);

    in_addr udp_address_v4;
    udp_address_v4.s_addr = htonl(DEFAULT_ADDRESSES_TARGET_IP);
    if (*ipv4_option &&
        !inet_pton(AF_INET, udp_address_v4_str.c_str(), &udp_address_v4))
    {
        std::cerr << "Invalid IPv4 address supplied: " << udp_address_v4_str
                  << std::endl;
    }

    // The value from default_addresses.h is designed for LWIP which EC uses,
    // and needs to be translated to network byte order.
    in6_addr udp_address_v6;
    uint32_t default_addr_6[4] = DEFAULT_ADDRESSES_TARGET_IP6;

    auto htonl_inplace = [](uint32_t& i) { i = htonl(i); };
    std::for_each(std::begin(default_addr_6), std::end(default_addr_6),
                  htonl_inplace);
    std::memcpy(udp_address_v6.s6_addr, default_addr_6, sizeof(default_addr_6));

    if (*ipv6_option &&
        !inet_pton(AF_INET6, udp_address_v6_str.c_str(), &udp_address_v6))
    {
        std::cerr << "Invalid IPv6 address supplied: " << udp_address_v6_str
                  << std::endl;
    }

    log<level::INFO>("Start Nemora...");
    Nemora nemora(iface_name, udp_address_v4, udp_address_v6);

    // Install a signal handler
    std::signal(SIGINT, signal_handler);

    std::thread udp(NemoraUdpPoll, &nemora);

    udp.join();

    return EXIT_SUCCESS;
}
