#pragma once
#include <flasher/mod.hpp>
#include <stdplus/types.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace flasher
{

class Mutate
{
  public:
    virtual ~Mutate() = default;
    virtual void forward(stdplus::span<std::byte> data, size_t offset) = 0;
    virtual void reverse(stdplus::span<std::byte> data, size_t offset) = 0;
};

class NestedMutate : public Mutate
{
  public:
    std::vector<std::unique_ptr<Mutate>> mutations;

    void forward(stdplus::span<std::byte> data, size_t offset);
    void reverse(stdplus::span<std::byte> data, size_t offset);
};

class MutateType : public ModType<Mutate>
{
  public:
    virtual std::unique_ptr<Mutate> open(const ModArgs& args) = 0;
};

extern ModTypeMap<MutateType> mutateTypes;
std::unique_ptr<Mutate> openMutate(const ModArgs& args);

} // namespace flasher
