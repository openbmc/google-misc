#include <flasher/mutate/rot128.hpp>

namespace flasher
{
namespace mutate
{

void Rot128::forward(stdplus::span<std::byte> data, size_t)
{
    for (auto& c : data)
    {
        c ^= static_cast<std::byte>('\x80');
    }
}

void Rot128::reverse(stdplus::span<std::byte> data, size_t offset)
{
    forward(data, offset);
}

class Rot128Type : public MutateType
{
  public:
    std::unique_ptr<Mutate> open(const ModArgs& args) override
    {
        if (args.arr.size() != 1)
        {
            throw std::invalid_argument("Requires no arguments");
        }
        return std::make_unique<Rot128>();
    }

    void printHelp() const override
    {
        fmt::print(stderr, "  `rot128` mutator (no options)\n");
    }
};

void registerRot128() __attribute__((constructor));
void registerRot128()
{
    mutateTypes.emplace("rot128", std::make_unique<Rot128Type>());
}

void unregisterRot128() __attribute__((destructor));
void unregisterRot128()
{
    mutateTypes.erase("rot128");
}

} // namespace mutate
} // namespace flasher
