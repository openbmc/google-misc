#include "dbus_command.hpp"

#include "dbus_command_impl.hpp"

#include <fmt/format.h>

#include <stdplus/cancel.hpp>

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

stdplus::Cancel
    DbusCommandImpl::SendHostCommand(std::string_view hothId,
                                     const std::vector<uint8_t>& cmd, Cb&& cb)
{
    auto req = newHothdCall(hothId, "SendHostCommand");
    req.append(cmd);
    auto cHothId = std::string(hothId);
    return stdplus::Cancel(new BusCall(req.call_async(
        [cb = std::move(cb), hothId = std::move(cHothId)](
            sdbusplus::message::message m) mutable noexcept {
            if (m.is_method_error())
            {
                auto err = m.get_error();
                fmt::print(stderr, "SendHostCommand failed on `{}`: {}: {}\n",
                           hothId, err->name, err->message);
                cb(std::nullopt);
                return;
            }
            try
            {
                std::vector<uint8_t> rsp;
                m.read(rsp);
                cb(std::move(rsp));
            }
            catch (const std::exception& e)
            {
                fmt::print(stderr,
                           "SendHostCommand failed unpacking on `{}`: {}\n",
                           hothId, e.what());
                cb(std::nullopt);
            }
        },
        asyncCallTimeout)));
}

} // namespace internal

} // namespace ipmi_hoth
