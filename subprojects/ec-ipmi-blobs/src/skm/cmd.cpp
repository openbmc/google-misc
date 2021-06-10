#include <ec-ipmi-blobs/skm/cmd.hpp>
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

stdplus::Cancel readKey(
    Cmd& cmd, uint8_t idx,
    fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)> cb) noexcept
{
    CmdParamsSKMKey req = {};
    req.subcmd = subcmdRead;
    req.idx = idx;
    auto cmdcb =
        [cb = std::move(cb)](uint8_t res,
                             stdplus::span<std::byte> bytes) mutable noexcept {
            // Non-zero error codes should be returned to sub handler
            if (res != 0)
            {
                cb(res, stdplus::span<std::byte>{});
                return;
            }

            try
            {
                auto& rsp = stdplus::raw::refFrom<CmdParamsSKMKey>(bytes);
                cb(res, stdplus::raw::asSpan<std::byte>(rsp.key));
            }
            catch (...)
            {
                // Handle any decoding errors
                cb(resInvalidRsp, stdplus::span<std::byte>{});
            }
        };
    return cmd.exec(cmdSkmKey, 0, stdplus::raw::asSpan<std::byte>(req),
                    std::move(cmdcb));
}

stdplus::Cancel writeKey(Cmd& cmd, uint8_t idx,
                         stdplus::span<const std::byte> data,
                         fu2::unique_function<void(uint8_t)> cb) noexcept
{
    CmdParamsSKMKey req = {};
    req.subcmd = subcmdWrite;
    req.idx = idx;
    std::memcpy(req.key, data.data(), std::min(sizeof(req.key), data.size()));
    return cmd.exec(
        cmdSkmKey, 0, stdplus::raw::asSpan<std::byte>(req),
        [cb = std::move(cb)](uint8_t res,
                             stdplus::span<std::byte>) mutable noexcept {
            cb(res);
        });
}

} // namespace skm
} // namespace ec
} // namespace blobs
