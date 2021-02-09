#pragma once

#include <sys/types.h>

namespace net
{

class SockIO
{
  public:
    SockIO() = default;
    explicit SockIO(int sockfd) : sockfd_{sockfd}
    {}
    virtual ~SockIO();

    int get_sockfd() const
    {
        return sockfd_;
    }

    virtual int write(const void* buf, size_t len);

    virtual int close();

    virtual int recv(void* buf, size_t maxlen);

  protected:
    int sockfd_ = -1;
};

} // namespace net
