#include <netinet/in.h>
#include <sys/socket.h>

#include <ec-ipmi-blobs/cmd_net.hpp>
#include <stdplus/fd/managed.hpp>
#include <stdplus/io_uring.hpp>
#include <stdplus/raw.hpp>
#include <stdplus/util/cexec.hpp>

#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{
namespace ec
{

TEST(UringSupport, Ok)
{
    io_uring r;
    if (io_uring_queue_init(1, &r, 0) == -ENOSYS)
    {
        // Not supported, skip running this test
        exit(77);
    }
    io_uring_queue_exit(&r);
}

constexpr uint8_t cmdHello = 1;
constexpr uint8_t resInvalidCmd = 1;

class PingServer : public stdplus::IoUring::CQEHandler
{
  public:
    PingServer(stdplus::IoUring& ring, size_t retries_needed = 0) :
        ring(&ring), retries_needed(retries_needed),
        sock(CHECK_ERRNO(socket(AF_INET6, SOCK_STREAM, 0), "socket"))
    {
        // Allow quick port re-use
        int optval = 1;
        CHECK_ERRNO(setsockopt(sock.get(), SOL_SOCKET, SO_REUSEPORT, &optval,
                               sizeof(optval)),
                    "setsockopt");

        // Listen on localhost:4040
        sockaddr_in6 addr = {};
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(4040);
        addr.sin6_addr.s6_addr[15] = 1;
        CHECK_ERRNO(
            bind(sock.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)),
            "bind");
        CHECK_ERRNO(listen(sock.get(), 1), "listen");
        queueAccept();
    }
    ~PingServer()
    {
        ring->cancelHandler(*this);
        ring->process();
    }

    void handleCQE(io_uring_cqe& cqe) noexcept override
    {
        if (cqe.res == -ECANCELED)
        {
            return;
        }
        if (cqe.res < 0)
        {
            std::terminate();
        }
        new PingHandler(*ring, stdplus::ManagedFd(std::move(cqe.res)),
                        connections++ < retries_needed);
        queueAccept();
    }

    size_t connections = 0;

  private:
    stdplus::IoUring* ring;
    size_t retries_needed;
    stdplus::ManagedFd sock;

    void queueAccept()
    {
        auto& sqe = ring->getSQE();
        io_uring_prep_accept(&sqe, sock.get(), nullptr, nullptr, 0);
        ring->setHandler(sqe, this);
        ring->submit();
    }

    class PingHandler : CQEHandler
    {
      public:
        PingHandler(stdplus::IoUring& ring, stdplus::ManagedFd&& sock,
                    bool should_fail) :
            ring(&ring),
            sock(std::move(sock)), should_fail(should_fail)
        {
            queueRead();
        }

        void handleCQE(io_uring_cqe& cqe) noexcept override
        {
            if (should_fail || cqe.res == 0 || cqe.res == -ECANCELED)
            {
                delete this;
                return;
            }
            if (cqe.res < 0)
            {
                std::terminate();
            }
            if (!reading)
            {
                queueRead();
                return;
            }
            try
            {
                auto req = stdplus::span<std::byte>(buf.begin(), cqe.res);
                auto reqhdr = stdplus::raw::extract<detail::CmdNetRequest>(req);
                req = req.subspan(0, reqhdr.params_size);
                auto rsp = stdplus::span<std::byte>(buf);
                auto& rsphdr =
                    stdplus::raw::extractRef<detail::CmdNetResponse>(rsp);
                if (reqhdr.cmd == cmdHello)
                {
                    rsphdr.res = 0;
                    rsphdr.rsp_size = req.size();
                    std::memmove(rsp.data(), req.data(), req.size());
                }
                else
                {
                    rsphdr.res = resInvalidCmd;
                    rsphdr.rsp_size = 0;
                }
                reading = false;
                auto& sqe = ring->getSQE();
                io_uring_prep_send(&sqe, sock.get(), buf.data(),
                                   sizeof(rsphdr) + rsphdr.rsp_size, 0);
                ring->setHandler(sqe, this);
                ring->submit();
            }
            catch (...)
            {
                delete this;
            }
        }

      private:
        stdplus::IoUring* ring;
        stdplus::ManagedFd sock;
        bool should_fail, reading;
        std::array<std::byte, 1024> buf = {};

        void queueRead()
        {
            reading = true;
            auto& sqe = ring->getSQE();
            io_uring_prep_recv(&sqe, sock.get(), buf.data(), buf.size(), 0);
            ring->setHandler(sqe, this);
            ring->submit();
        }
    };
};

void doHello(stdplus::IoUring& ring, CmdNet& cmd)
{
    std::array<std::byte, 4> req;
    for (size_t i = 0; i < req.size(); ++i)
    {
        req[i] = static_cast<std::byte>(i);
    }
    bool running = true;
    auto cancel = cmd.exec(
        cmdHello, 0, req, [&](uint8_t res, stdplus::span<std::byte> rsp) {
            running = false;
            EXPECT_EQ(res, 0);
            ASSERT_EQ(rsp.size(), req.size());
            EXPECT_EQ(std::memcmp(rsp.data(), req.data(), req.size()), 0);
        });
    while (running)
    {
        ring.process();
    }
}

TEST(CmdNetTest, SingleCmd)
{
    stdplus::IoUring ring;
    PingServer srv(ring);
    CmdNet cmd(ring, "::1");
    doHello(ring, cmd);
    EXPECT_EQ(srv.connections, 1);
    doHello(ring, cmd);
    EXPECT_EQ(srv.connections, 2);
}

TEST(CmdNetTest, Retries)
{
    stdplus::IoUring ring;
    PingServer srv(ring, /*retries_needed=*/2);
    CmdNet cmd(ring, "::1", /*max_retries=*/3,
               /*cmd_timeout=*/std::chrono::milliseconds(500),
               /*ec_timeout=*/std::chrono::milliseconds(100));
    doHello(ring, cmd);
    EXPECT_EQ(srv.connections, 3);
}

TEST(CmdNetTest, CmdError)
{
    stdplus::IoUring ring;
    PingServer srv(ring);
    CmdNet cmd(ring, "::1");
    bool running = true;
    auto cancel =
        cmd.exec(255, 0, {}, [&](uint8_t res, stdplus::span<std::byte>) {
            running = false;
            EXPECT_EQ(res, resInvalidCmd);
        });
    while (running)
    {
        ring.process();
    }
    EXPECT_EQ(srv.connections, 1);
}

TEST(CmdNetTest, MaxAttempts)
{
    // Test that we only attempt a limited number of partial connections before
    // we stop trying
    stdplus::IoUring ring;
    PingServer srv(ring, /*retries_needed=*/3);
    CmdNet cmd(ring, "::1", /*max_retries=*/3);
    bool running = true;
    auto cancel =
        cmd.exec(cmdHello, 0, {}, [&](uint8_t res, stdplus::span<std::byte>) {
            running = false;
            EXPECT_EQ(res, resBusErr);
        });
    while (running)
    {
        ring.process();
    }
    EXPECT_EQ(srv.connections, 3);
}

TEST(CmdNetTest, Timeout)
{
    // Test that we time out after the designated time, by not actually
    // listening with a ping server so connect will indefinitely fail
    stdplus::IoUring ring;
    CmdNet cmd(ring, "::1", /*max_retries=*/1,
               /*cmd_timeout=*/std::chrono::milliseconds(300),
               /*ec_timeout=*/std::chrono::milliseconds(500),
               /*backoff_time=*/std::chrono::milliseconds(50));
    bool running = true;
    auto cancel =
        cmd.exec(cmdHello, 0, {}, [&](uint8_t res, stdplus::span<std::byte>) {
            running = false;
            EXPECT_EQ(res, resBusErr);
        });
    while (running)
    {
        ring.process();
    }
}

TEST(CmdNetTest, CancelEarly)
{
    stdplus::IoUring ring;
    CmdNet cmd(ring, "::1");
    bool running = true;
    // Implicitly drop the cancelation handle by not assigning it, causing a
    // cancel prior to connecting
    cmd.exec(cmdHello, 0, {}, [&](uint8_t res, stdplus::span<std::byte>) {
        running = false;
        EXPECT_EQ(res, resBusErr);
    });
    while (running)
    {
        ring.process();
    }
}

TEST(CmdNetTest, RingExitEarly)
{
    // Ensure that the command safely cleans up if the ring is destroyed before
    // it
    std::unique_ptr<CmdNet> cmd;
    bool called = false;
    {
        stdplus::IoUring ring;
        cmd = std::make_unique<CmdNet>(ring, "::1");
        cmd->exec(cmdHello, 0, {}, [&](uint8_t res, stdplus::span<std::byte>) {
            called = true;
            EXPECT_EQ(res, resBusErr);
        });
    }
    EXPECT_TRUE(called);
}

} // namespace ec
} // namespace blobs
