#include <flasher/mutate.hpp>

namespace flasher
{

void NestedMutate::forward(stdplus::span<std::byte> data, size_t offset)
{
    for (auto it = mutations.begin(); it != mutations.end(); ++it)
    {
        (*it)->forward(data, offset);
    }
}

void NestedMutate::reverse(stdplus::span<std::byte> data, size_t offset)
{
    for (auto it = mutations.rbegin(); it != mutations.rend(); ++it)
    {
        (*it)->reverse(data, offset);
    }
}

ModTypeMap<MutateType> mutateTypes;

std::unique_ptr<Mutate> openMutate(const ModArgs& args)
{
    return openMod<Mutate>(mutateTypes, args);
}

} // namespace flasher
