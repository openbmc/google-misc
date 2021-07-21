#pragma once
#include <flasher/mutate.hpp>

namespace flasher
{
namespace mutate
{

class Rot128 : public Mutate
{
  public:
    void forward(stdplus::span<std::byte> data, size_t offset) override;
    void reverse(stdplus::span<std::byte> data, size_t offset) override;
};

} // namespace mutate
} // namespace flasher
