#pragma once

#include "net_iface.h"
#include "net_sockio.h"

#include <sys/socket.h>

#include <cstddef>
#include <cstring>

namespace ncsi
{

class SockIO : public net::SockIO
{
  public:
    SockIO() = default;

    explicit SockIO(int sockfd) : net::SockIO(sockfd)
    {}

    // This function creates a raw socket and initializes sockfd_.
    // If the default constructor for this class was used,
    // this function MUST be called before the object can be used
    // for anything else.
    int init();

    // Since raw packet socket is used for NC-SI, it needs to be bound
    // to the interface. This function needs to be called after init,
    // before the socket it used for communication.
    int bind_to_iface(const net::IFaceBase& iface);

    // Applies a filter to the interface to ignore VLAN tagged packets
    int filter_vlans();

    // Non-blocking version of recv. Uses poll with timeout.
    int recv(void* buf, size_t maxlen) override;

  private:
    struct sockaddr_ll sock_addr_;
    const int kpoll_timeout_ = 10;
};

} // namespace ncsi
