#pragma once

#include <net_iface.h>

#include <vector>

namespace mock
{

class IFace : public net::IFaceBase
{
  public:
    IFace() : net::IFaceBase("mock0")
    {}
    explicit IFace(const std::string& name) : net::IFaceBase(name)
    {}
    int bind_sock(int sockfd, struct sockaddr_ll* saddr) const override;

    mutable std::vector<int> bound_socks;
    int index;
    mutable short flags = 0;

  private:
    int ioctl_sock(int sockfd, int request, struct ifreq* ifr) const override;
    int ioctl(int request, struct ifreq* ifr) const override;
};

} // namespace mock
