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

#include "socket_manager.hpp"

#include "serializer.hpp"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>

#include <cstring>
#include <format>

using phosphor::logging::level;
using phosphor::logging::log;

SocketManager::~SocketManager()
{
    std::lock_guard<std::mutex> lock(open_sockets_lock_);
    for (auto fd : open_sockets_)
    {
        close(fd);
    }
}

void SocketManager::SendDatagram(const NemoraDatagram* bcast)
{
    std::string serialized = Serializer::Serialize(bcast);

    // Create socket
    auto fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        log<level::ERR>("SocketManager::SendDatagram: Couldn't open socket");
    }
    TrackSocket(fd);

    // Because we aren't sure whether the v6 or v4 target IP will be present,
    // we send UDP packets to both. This puts us at feature parity with EC.

    // Send serialized data (v6)
    auto addr6 = reinterpret_cast<const sockaddr*>(&bcast->destination6);
    auto err = sendto(fd, serialized.c_str(), serialized.length(), 0, addr6,
                      sizeof(bcast->destination6));
    if (err < 0)
    {
        log<level::ERR>(
            std::format("SocketManager::SendDatagram: Couldn't sendto "
                        "socket (IPv6): {}",
                        std::strerror(errno))
                .c_str());
    }

    // Send serialized data (v4)
    auto addr4 = reinterpret_cast<const sockaddr*>(&bcast->destination);
    err = sendto(fd, serialized.c_str(), serialized.length(), 0, addr4,
                 sizeof(bcast->destination));
    if (err < 0)
    {
        log<level::ERR>(
            std::format("SocketManager::SendDatagram: Couldn't sendto "
                        "socket (IPv4): {}",
                        std::strerror(errno))
                .c_str());
    }

    CloseSocketSafely(fd);
}

void SocketManager::CloseSocketSafely(int fd)
{
    std::lock_guard<std::mutex> lock(open_sockets_lock_);
    if (open_sockets_.find(fd) != open_sockets_.end())
    {
        close(fd);
        open_sockets_.erase(fd);
    }
}

void SocketManager::TrackSocket(int fd)
{
    std::lock_guard<std::mutex> lock(open_sockets_lock_);
    open_sockets_.insert(fd);
}
