#include "net_sockio.h"

#include <sys/socket.h>
#include <unistd.h>

namespace net
{

int SockIO::close()
{
    int ret = 0;
    if (sockfd_ >= 0)
    {
        ret = ::close(sockfd_);
        sockfd_ = -1;
    }

    return ret;
}

int SockIO::write(const void* buf, size_t len)
{
    return ::write(sockfd_, buf, len);
}

int SockIO::recv(void* buf, size_t maxlen)
{
    return ::recv(sockfd_, buf, maxlen, 0);
}

SockIO::~SockIO()
{
    SockIO::close();
}

} // namespace net
