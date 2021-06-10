#pragma once
#include <ec-ipmi-blobs/cmd.hpp>
#include <function2/function2.hpp>
#include <stdplus/types.hpp>

#include <cstddef>

namespace blobs
{
namespace ec
{
namespace skm
{

constexpr size_t numKeys = 4;
constexpr size_t keySize = 64;

Cancel readKey(
    Cmd& cmd, uint8_t idx,
    fu2::unique_function<void(uint8_t, stdplus::span<std::byte>)> cb) noexcept;

Cancel writeKey(Cmd& cmd, uint8_t idx, stdplus::span<const std::byte> data,
                fu2::unique_function<void(uint8_t)> cb) noexcept;

} // namespace skm
} // namespace ec
} // namespace blobs
