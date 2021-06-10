#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <ec-ipmi-blobs/cmd_net.hpp>
#include <stdplus/fd/managed.hpp>
#include <stdplus/raw.hpp>
#include <stdplus/util/cexec.hpp>

namespace blobs
{
namespace ec
{

CmdNet::CmdNet(IoUring& ring, const char* addrstr, size_t max_attempts,
               std::chrono::milliseconds cmd_timeout,
               std::chrono::milliseconds ec_timeout,
               std::chrono::milliseconds backoff_time) :
    ring(&ring),
    max_attempts(max_attempts), cmd_timeout(cmd_timeout),
    ec_timeout(ec_timeout), backoff_time(backoff_time)
{
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(4094);
    addr.sin6_flowinfo = 0;
    CHECK_ERRNO(inet_pton(AF_INET6, addrstr, &addr.sin6_addr), "inet_pton");
    addr.sin6_scope_id = 0;
}

constexpr size_t bufSize = 4096;

Cancel CmdNet::exec(
    uint8_t cmd, uint8_t ver, stdplus::span<const std::byte> params,
    fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)> cb) noexcept
{
    std::vector<std::byte> req(sizeof(detail::CmdRequest) + params.size());
    auto payload = stdplus::span<std::byte>(req);
    auto& hdr = stdplus::raw::extractRef<detail::CmdRequest>(payload);
    hdr.cmd = cmd;
    hdr.ver = ver;
    hdr.params_size = params.size();
    std::memcpy(payload.data(), params.data(), payload.size());
    return Cancel(new CmdHandler(*this, std::move(req), std::move(cb)));
}

CmdNet::CmdHandler::CmdHandler(
    CmdNet& cmd, std::vector<std::byte>&& req,
    fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>&& cb) noexcept
    :
    ring(cmd.ring),
    addr(cmd.addr), max_attempts(cmd.max_attempts), attempts(0),
    ec_timeout(cmd.ec_timeout), backoff_time(cmd.backoff_time), next_backoff(),
    last_connect(),
    cmd_timeout(std::chrono::steady_clock::now() + cmd.cmd_timeout),
    acb(std::move(cb), resBusErr, stdplus::span<std::byte>{}),
    outstanding_sqe(false), req(std::move(req))
{
    connect();
}

void CmdNet::CmdHandler::cancel() noexcept
{
    if (outstanding_sqe)
    {
        state = State::Canceled;
        ring->cancelHandler(*this);
    }
    else
    {
        delete this;
    }
}

static __kernel_timespec chronoToTs(std::chrono::nanoseconds t)
{
    __kernel_timespec ts;
    ts.tv_sec = std::chrono::floor<std::chrono::seconds>(t).count();
    ts.tv_nsec = (t % std::chrono::seconds(1)).count();
    return ts;
}

void CmdNet::CmdHandler::abort() noexcept
{
    state = State::Abort;
    auto sqe = ring->getSqe();
    io_uring_prep_nop(sqe);
    submitSqe(*sqe);
}

void CmdNet::CmdHandler::connect() noexcept
{
    if (attempts >= max_attempts)
    {
        abort();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    std::chrono::nanoseconds t = {};
    __kernel_timespec ts;
    if (last_connect + next_backoff > now)
    {
        t = last_connect + next_backoff - now;
        if (cmd_timeout - now < t)
        {
            abort();
            return;
        }
        auto sqe = ring->getSqe();
        ts = chronoToTs(t);
        io_uring_prep_timeout(sqe, &ts, 0, 0);
        sqe->flags |= IOSQE_IO_HARDLINK;
    }
    last_connect = now + t;
    state = State::Connecting;
    sock = stdplus::ManagedFd(
        CHECK_ERRNO(socket(AF_INET6, SOCK_STREAM, 0), "socket"));
    auto sqe = ring->getSqe();
    io_uring_prep_connect(sqe, sock->get(), reinterpret_cast<sockaddr*>(&addr),
                          sizeof(addr));
    submitSqe(*sqe);
}

void CmdNet::CmdHandler::send() noexcept
{
    if (completed_bytes >= req.size())
    {
        state = State::Receiving;
        completed_bytes = 0;
        receive();
        return;
    }
    auto sqe = ring->getSqe();
    io_uring_prep_send(sqe, sock->get(), req.data() + completed_bytes,
                       req.size() - completed_bytes, 0);
    submitSqe(*sqe);
}

void CmdNet::CmdHandler::receive() noexcept
{
    if (completed_bytes >= sizeof(detail::CmdResponse))
    {
        auto payload = stdplus::span<std::byte>(rsp);
        auto& rsp = stdplus::raw::extractRef<detail::CmdResponse>(payload);
        if (rsp.rsp_size + sizeof(detail::CmdResponse) <= completed_bytes)
        {
            acb(rsp.res, payload.subspan(0, rsp.rsp_size));
            return;
        }
    }
    rsp.resize(completed_bytes + bufSize);
    auto sqe = ring->getSqe();
    io_uring_prep_recv(sqe, sock->get(), rsp.data() + completed_bytes, bufSize,
                       0);
    submitSqe(*sqe);
}

void CmdNet::CmdHandler::handleCqe(io_uring_cqe& cqe) noexcept
{
    outstanding_sqe = false;
    switch (state)
    {
        case State::Connecting:
            if (cqe.res < 0)
            {
                connect();
                next_backoff =
                    std::min(ec_timeout, next_backoff * 2 + backoff_time);
                break;
            }
            next_backoff = {};
            state = State::Sending;
            completed_bytes = 0;
            send();
            break;
        case State::Sending:
            if (cqe.res <= 0)
            {
                attempts++;
                connect();
                break;
            }
            completed_bytes += cqe.res;
            send();
            break;
        case State::Receiving:
            if (cqe.res <= 0)
            {
                attempts++;
                connect();
                break;
            }
            completed_bytes += cqe.res;
            receive();
            break;
        case State::Canceled:
            delete this;
            break;
        case State::Abort:
            acb(resBusErr, stdplus::span<std::byte>{});
            break;
    }
}

void CmdNet::CmdHandler::submitSqe(io_uring_sqe& sqe) noexcept
{
    auto left = cmd_timeout - std::chrono::steady_clock::now();
    if (left < left.zero())
    {
        state = State::Abort;
        io_uring_prep_nop(&sqe);
    }
    else
    {
        sqe.flags |= IOSQE_IO_LINK;
        auto t = std::min<std::chrono::nanoseconds>(ec_timeout, left);
        auto ts = chronoToTs(t);
        io_uring_prep_link_timeout(ring->getSqe(), &ts, 0);
    }
    ring->setHandler(sqe, this);
    outstanding_sqe = true;
    ring->submit();
}

} // namespace ec
} // namespace blobs
