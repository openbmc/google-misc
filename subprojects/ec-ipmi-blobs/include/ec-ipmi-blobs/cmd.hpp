#pragma once
#include <boost/endian/arithmetic.hpp>
#include <ec-ipmi-blobs/util.hpp>
#include <function2/function2.hpp>
#include <stdplus/types.hpp>

#include <cstddef>

namespace blobs
{
namespace ec
{

namespace detail
{

struct CmdRequest
{
    uint8_t proto_ver = 1;
    uint8_t cmd;
    uint8_t ver;
    uint8_t rsv0;
    boost::endian::little_uint32_t params_size;
};

struct CmdResponse
{
    uint8_t res;
    uint8_t rsv0[3];
    boost::endian::little_uint32_t rsp_size;
};

} // namespace detail

constexpr uint8_t resInvalidRsp = 5;
constexpr uint8_t resBusErr = 15;

class Cmd
{
  public:
    using Cb = fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>;

    virtual ~Cmd() = default;
    virtual Cancel exec(uint8_t cmd, uint8_t ver,
                        stdplus::span<const std::byte> params,
                        Cb cb) noexcept = 0;
};

} // namespace ec
} // namespace blobs
