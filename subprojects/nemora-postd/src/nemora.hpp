#pragma once

#include "host_manager.hpp"
#include "socket_manager.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class Nemora
{
  public:
    /**
     * Constructs a Nemora object.
     * - iface_name: The networking interface to use (eg. eth0)
     * - ipv4: Target IPv4 address for UDP communication, i.e., POST streaming.
     * - ipv6: Target IPv6 address for UDP communication, i.e., POST streaming.
     */
    Nemora(const std::string& iface_name, const in_addr ipv4,
           const in6_addr ipv6);

    /**
     * Construct uninitialized Nemora object
     */
    Nemora();

    /**
     * Cancels polling threads and destructs Nemora object.
     */
    ~Nemora() = default;

    /**
     * Loops collecting the current state of event_data_ and sending via UDP.
     */
    void UdpPoll();

  private:
    /**
     * Initialize event_data_ with default values.
     * This is used by constructors.
     */
    void InitEventData();

    /**
     * Fetches MAC from host
     * - mac: out-parameter for host mac address
     * - iface_path: DBus path to network interface, typically
     *   IFACE_ROOT + iface_path_.
     *
     * - returns: true if address was populated correctly, false if error
     */
    bool GetMacAddr(MacAddr* mac, const std::string& iface_path);

    /**
     * Converts from string to struct
     * - mac_addr: string of format MAC_FORMAT
     * - mac: out-parameter with MAC from mac_addr populated. must be allocated
     * by caller
     *
     * - returns: true if mac_addr was correct format, false otherwise
     */
    bool ParseMac(const std::string& mac_addr, MacAddr* mac);

    /**
     * Update event_data_ from host.
     * - postcodes: list of postcodes polled.
     *   Forced to bind to temporary to avoid copying.
     */
    void UpdateEventData(std::vector<uint64_t>&& postcodes);

    NemoraEvent event_data_ = {};
    std::mutex event_data_mutex_;

    SocketManager socketManager_;
    HostManager hostManager_;
    const std::string iface_path_;
};
