#pragma once
#include "nemora_types.hpp"

#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

using std::int32_t;
using std::size_t;
using std::uint64_t;

class Serializer
{
  public:
    static std::string Serialize(const NemoraDatagram* dgram);

  private:
    static std::string SerializeEvent(const NemoraEvent* event);

    static constexpr uint64_t NEMORA_EVENT_PB_MAGIC = 0x890ebd38ec325800;
};
