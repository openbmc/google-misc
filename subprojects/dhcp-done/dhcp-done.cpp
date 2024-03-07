// Copyright 2022 Google LLC
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

#include "../config.h"

#include "file-io.hpp"

#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/ops.hpp>
#include <stdplus/print.hpp>

using namespace std::string_view_literals;

// A privileged port that is reserved for querying BMC DHCP completion.
// This is well known by the clients querying the status.
constexpr uint16_t kListenPort = 23;

stdplus::ManagedFd createListener()
{
    using namespace stdplus::fd;
    auto sock = socket(SocketDomain::INet6, SocketType::Stream,
                       SocketProto::TCP);
    setFileFlags(sock, getFileFlags(sock).set(stdplus::fd::FileFlag::NonBlock));
    sockaddr_in6 addr = {};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(kListenPort);
    bind(sock, addr);
    listen(sock, 10);
    return sock;
}

int main()
{
    try
    {
        auto listener = createListener();
        auto event = sdeventplus::Event::get_default();
        sdeventplus::source::IO do_accept(
            event, listener.get(), EPOLLIN | EPOLLET,
            [&](sdeventplus::source::IO&, int, uint32_t) {
            while (auto fd = stdplus::fd::accept(listener))
            {
                dhcp_status status;
                std::string data;
                if (file_read(DHCP_STATUS_FILE_PATH, status))
                {
                    stdplus::println(stderr, "Failed to read status");
                    // we don't want to fail the upgrade process, set the status
                    // to ONGOING
                    status.code = 2;
                    status.message = "Unknown status";
                }
                data.push_back(status.code);
                data.append(status.message);
                stdplus::fd::sendExact(*fd, data, stdplus::fd::SendFlags(0));
            }
        });
        return event.loop();
    }
    catch (const std::exception& e)
    {
        stdplus::println(stderr, "Failed: {}", e.what());
        return 1;
    }
}
