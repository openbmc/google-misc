#include <flasher/ops.hpp>

namespace flasher
{
namespace ops
{

void automatic(Device&, size_t, File&, size_t, Mutate&, size_t,
               std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void read(Device&, size_t, File&, size_t, Mutate&, size_t,
          std::optional<size_t>)
{
    throw std::runtime_error("Not implemented");
}

void write(Device&, size_t, File&, size_t, Mutate&, size_t,
           std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void erase(Device&, size_t, size_t, std::optional<size_t>, bool)
{
    throw std::runtime_error("Not implemented");
}

void verify(Device&, size_t, File&, size_t, Mutate&, size_t,
            std::optional<size_t>)
{
    throw std::runtime_error("Not implemented");
}

} // namespace ops
} // namespace flasher
