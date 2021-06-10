#include <ec-ipmi-blobs/skm/cmd.hpp>
#include <ec-ipmi-blobs/util.hpp>
#include <stdplus/raw.hpp>

#include <algorithm>
#include <utility>

namespace blobs
{
namespace ec
{
namespace skm
{

constexpr uint8_t cmdSkmKey = 0xca;

constexpr uint8_t subcmdWrite = 1;
constexpr uint8_t subcmdRead = 2;

struct CmdParamsSKMKey
{
    uint8_t subcmd;
    uint8_t idx;
    uint8_t key[keySize];
};

Cancel readKey(
    Cmd& cmd, uint8_t idx,
    fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)> cb) noexcept
{
    auto acb =
        alwaysCallOnce(std::move(cb), resBusErr, stdplus::span<std::byte>{});
    CmdParamsSKMKey req = {};
    req.subcmd = subcmdRead;
    req.idx = idx;
    auto cmdcb =
        [cb = std::move(acb)](uint8_t res,
                              stdplus::span<std::byte> bytes) mutable noexcept {
            // Move callback to local scope to guarantee it executes
            auto acb = std::move(cb);

            // Non-zero error codes should be returned to sub handler
            if (res != 0)
            {
                acb(res, stdplus::span<std::byte>{});
                return;
            }

            try
            {
                auto& rsp = stdplus::raw::refFrom<CmdParamsSKMKey>(bytes);
                acb(res, stdplus::raw::asSpan<std::byte>(rsp.key));
            }
            catch (...)
            {
                // Handle any decoding errors
                acb(resInvalidRsp, stdplus::span<std::byte>{});
            }
        };
    return cmd.exec(cmdSkmKey, 0, stdplus::raw::asSpan<std::byte>(req),
                    std::move(cmdcb));
}

Cancel writeKey(Cmd& cmd, uint8_t idx, stdplus::span<const std::byte> data,
                fu2::unique_function<void(uint8_t)> cb) noexcept
{
    auto acb = alwaysCallOnce(std::move(cb), resBusErr);
    CmdParamsSKMKey req = {};
    req.subcmd = subcmdWrite;
    req.idx = idx;
    std::memcpy(req.key, data.data(), std::min(sizeof(req.key), data.size()));
    return cmd.exec(
        cmdSkmKey, 0, stdplus::raw::asSpan<std::byte>(req),
        [cb = std::move(acb)](uint8_t res,
                              stdplus::span<std::byte>) mutable noexcept {
            // Move callback to local scope to guarantee execution
            auto acb = std::move(cb);
            acb(res);
        });
}

} // namespace skm
} // namespace ec
} // namespace blobs
