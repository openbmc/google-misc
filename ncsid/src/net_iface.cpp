#include "net_iface.h"

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace net
{

IFaceBase::IFaceBase(const std::string& name) : name_{name}
{
    if (name.size() >= IFNAMSIZ)
    {
        throw std::length_error("Interface name is too long");
    }
}

int IFaceBase::get_index() const
{
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    int ret = ioctl(SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        return ret;
    }

    return ifr.ifr_ifindex;
}

int IFaceBase::set_sock_flags(int sockfd, short flags) const
{
    return mod_sock_flags(sockfd, flags, true);
}

int IFaceBase::clear_sock_flags(int sockfd, short flags) const
{
    return mod_sock_flags(sockfd, flags, false);
}

int IFaceBase::mod_sock_flags(int sockfd, short flags, bool set) const
{
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));

    int ret = ioctl_sock(sockfd, SIOCGIFFLAGS, &ifr);
    if (ret < 0)
    {
        return ret;
    }

    if (set)
    {
        ifr.ifr_flags |= flags;
    }
    else
    {
        ifr.ifr_flags &= ~flags;
    }
    return ioctl_sock(sockfd, SIOCSIFFLAGS, &ifr);
}

int IFace::ioctl_sock(int sockfd, int request, struct ifreq* ifr) const
{
    if (ifr == nullptr)
    {
        return -1;
    }

    /* Avoid string truncation. */
    size_t len = name_.length();
    if (len + 1 >= sizeof(ifr->ifr_name))
    {
        return -1;
    }

    std::memcpy(ifr->ifr_name, name_.c_str(), len);
    ifr->ifr_name[len] = 0;

    return ::ioctl(sockfd, request, ifr);
}

int IFace::bind_sock(int sockfd, struct sockaddr_ll* saddr) const
{
    if (saddr == nullptr)
    {
        return -1;
    }

    saddr->sll_ifindex = get_index();

    return bind(sockfd, reinterpret_cast<struct sockaddr*>(saddr),
                sizeof(*saddr));
}

int IFace::ioctl(int request, struct ifreq* ifr) const
{
    if (ifr == nullptr)
    {
        return -1;
    }

    int tempsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tempsock < 0)
    {
        return tempsock;
    }

    int ret = ioctl_sock(tempsock, request, ifr);
    close(tempsock);

    return ret;
}

} // namespace net
