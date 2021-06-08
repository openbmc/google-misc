#include "socket_manager.hpp"

#include "serializer.hpp"

#include <errno.h>
#include <fmt/format.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <phosphor-logging/log.hpp>

using fmt::format;
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
        log<level::ERR>(format("SocketManager::SendDatagram: Couldn't sendto "
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
        log<level::ERR>(format("SocketManager::SendDatagram: Couldn't sendto "
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
