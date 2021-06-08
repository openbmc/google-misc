#include "nemora.hpp"

#include <netinet/in.h>
#include <time.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <variant>

#include "default_addresses.h"

using phosphor::logging::level;
using phosphor::logging::log;
using sdbusplus::exception::SdBusError;

constexpr auto MAC_FORMAT = "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx";
constexpr auto MAC_INTERFACE = "xyz.openbmc_project.Network.MACAddress";
constexpr auto NETWORK_INTERFACE = "xyz.openbmc_project.Network";
constexpr auto PROP_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto IFACE_ROOT = "/xyz/openbmc_project/network/";

bool Nemora::ParseMac(const std::string& mac_addr, MacAddr* mac)
{
    int ret =
        sscanf(mac_addr.c_str(), MAC_FORMAT, mac->octet, mac->octet + 1,
               mac->octet + 2, mac->octet + 3, mac->octet + 4, mac->octet + 5);
    return (ret == MAC_ADDR_SIZE);
}

bool Nemora::GetMacAddr(MacAddr* mac, const std::string& iface_path)
{
    if (mac == nullptr)
    {
        log<level::ERR>("Nemora::GetMacAddr MAC Address is nullptr");
        return false;
    }
    auto dbus = sdbusplus::bus::new_default();
    sdbusplus::message::message reply;

    try
    {
        auto networkd_call = dbus.new_method_call(
            NETWORK_INTERFACE, iface_path.c_str(), PROP_INTERFACE, "Get");
        networkd_call.append(MAC_INTERFACE, "MACAddress");

        reply = dbus.call(networkd_call);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>(
            "Nemora::GetMacAddr failed to call Network D-Bus interface");
        return false;
    }

    std::variant<std::string> result;
    reply.read(result);
    auto mac_addr = std::get<std::string>(result);
    if (!ParseMac(mac_addr, mac))
    {
        log<level::ERR>("Nemora::GetMacAddr Failed to parse MAC Address");
        return false;
    }
    return true;
}

void Nemora::InitEventData()
{
    event_data_.type = NemoraDatagramType::NemoraEvent;

    // UDP IPv4 addr for POST
    event_data_.destination.sin_family = AF_INET;
    event_data_.destination.sin_port = htons(DEFAULT_ADDRESSES_TARGET_PORT);

    // UDP IPv6 addr for POST
    event_data_.destination6.sin6_family = AF_INET6;
    event_data_.destination6.sin6_port = htons(DEFAULT_ADDRESSES_TARGET_PORT);
}

Nemora::Nemora()
{
    InitEventData();
}

Nemora::Nemora(const std::string& iface_name, const in_addr ipv4,
               const in6_addr ipv6) :
    socketManager_(),
    hostManager_(), iface_path_{std::string(IFACE_ROOT) + iface_name}
{
    InitEventData();
    event_data_.destination.sin_addr = ipv4;
    event_data_.destination6.sin6_addr = ipv6;
}

void Nemora::UdpPoll()
{
    auto postcodes = hostManager_.DrainPostcodes();

    // Don't bother updating if there is no POST code
    // EC supports a flag EC_NEMORA_UDP_CONFIG_MASK_PERIODIC to send
    // periodic updates, which is non-POR for gBMC for now.
    bool shouldBroadcast = !postcodes.empty();

    UpdateEventData(std::move(postcodes));

    log<level::INFO>("UpdateEventData gets called.");

    if (shouldBroadcast)
    {
        log<level::INFO>("Should broadcast");
        std::lock_guard<std::mutex> lock(event_data_mutex_);
        socketManager_.SendDatagram(static_cast<NemoraDatagram*>(&event_data_));
    }

    sleep(20);
}

void Nemora::UpdateEventData(std::vector<uint64_t>&& postcodes)
{
    MacAddr mac;
    GetMacAddr(&mac, iface_path_);

    std::lock_guard<std::mutex> lock(event_data_mutex_);

    memcpy(event_data_.mac, mac.octet, sizeof(MacAddr));

    event_data_.postcodes = std::move(postcodes);
    event_data_.sent_time_s = time(0);
}
