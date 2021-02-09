#include "net_iface_mock.h"

namespace mock
{

int IFace::bind_sock(int sockfd, struct sockaddr_ll*) const
{
    bound_socks.push_back(sockfd);
    return 0;
}

int IFace::ioctl_sock(int, int request, struct ifreq* ifr) const
{
    return ioctl(request, ifr);
}

int IFace::ioctl(int request, struct ifreq* ifr) const
{
    int ret = 0;
    switch (request)
    {
        case SIOCGIFINDEX:
            ifr->ifr_ifindex = index;
            break;
        case SIOCGIFFLAGS:
            ifr->ifr_flags = flags;
            break;
        case SIOCSIFFLAGS:
            flags = ifr->ifr_flags;
            break;
        default:
            ret = -1;
    }

    return ret;
}

} // namespace mock
