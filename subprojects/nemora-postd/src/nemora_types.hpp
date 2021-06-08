#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#define MAC_ADDR_SIZE 6

struct MacAddr
{
    std::uint8_t octet[MAC_ADDR_SIZE]; // network order
};

enum class NemoraDatagramType
{
    NemoraEvent,
};

/**
 * Encompasses all valid outbound UDP messages
 */
struct NemoraDatagram
{
    // destination
    sockaddr_in destination;
    sockaddr_in6 destination6;
    // type
    NemoraDatagramType type;
    std::vector<uint8_t> payload;
};

/**
 * Event information as broadcast to System Health Data Collector
 */
struct NemoraEvent : NemoraDatagram
{
    std::uint8_t mac[MAC_ADDR_SIZE];
    std::uint64_t sent_time_s;
    std::vector<uint64_t> postcodes;
};
