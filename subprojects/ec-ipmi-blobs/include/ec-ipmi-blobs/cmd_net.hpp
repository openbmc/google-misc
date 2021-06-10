#pragma once
#include <netinet/in.h>

#include <ec-ipmi-blobs/cmd.hpp>
#include <ec-ipmi-blobs/io_uring.hpp>
#include <ec-ipmi-blobs/util.hpp>
#include <stdplus/fd/managed.hpp>

#include <chrono>
#include <optional>
#include <vector>

namespace blobs
{
namespace ec
{

class CmdNet : public Cmd
{
  public:
    CmdNet(IoUring& ring, const char* addrstr, size_t max_attempts = 5,
           std::chrono::milliseconds cmd_timeout = std::chrono::seconds(120),
           std::chrono::milliseconds ec_timeout = std::chrono::seconds(10),
           std::chrono::milliseconds backoff_time =
               std::chrono::milliseconds(100));

    Cancel exec(uint8_t cmd, uint8_t ver, stdplus::span<const std::byte> params,
                fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>
                    cb) noexcept override;

  private:
    class CmdHandler : private IoUring::CqeHandler, public Cancelable
    {
      public:
        CmdHandler(
            CmdNet& cmd, std::vector<std::byte>&& req,
            fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>&&
                cb) noexcept;

        void cancel() noexcept override;

      private:
        IoUring* ring;
        sockaddr_in6 addr;
        size_t max_attempts, attempts;
        std::chrono::milliseconds ec_timeout, backoff_time, next_backoff;
        std::chrono::steady_clock::time_point last_connect, cmd_timeout;
        AlwaysCallOnce<
            fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>,
            uint8_t, stdplus::span<std::byte>>
            acb;
        bool outstanding_sqe;
        enum class State
        {
            Connecting,
            Sending,
            Receiving,
            Canceled,
            Abort,
        } state;
        size_t completed_bytes;
        std::vector<std::byte> req, rsp;
        std::optional<stdplus::ManagedFd> sock;

        void abort() noexcept;
        void connect() noexcept;
        void send() noexcept;
        void receive() noexcept;
        void handleCqe(io_uring_cqe&) noexcept override;
        void submitSqe(io_uring_sqe&) noexcept;
    };

    IoUring* ring;
    sockaddr_in6 addr;
    size_t max_attempts;
    std::chrono::milliseconds cmd_timeout, ec_timeout, backoff_time;
};

} // namespace ec
} // namespace blobs
