#pragma once
#include <function2/function2.hpp>
#include <stdplus/cancel.hpp>
#include <stdplus/types.hpp>

#include <cstddef>

namespace blobs
{
namespace ec
{

constexpr uint8_t resInvalidRsp = 5;
constexpr uint8_t resBusErr = 15;

class Cmd
{
  public:
    using Cb = fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)>;

    virtual ~Cmd() = default;
    virtual stdplus::Cancel exec(uint8_t cmd, uint8_t ver,
                                 stdplus::span<const std::byte> params,
                                 Cb cb) noexcept = 0;
};

} // namespace ec
} // namespace blobs
