// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "serializer.hpp"

#include "event_message.pb.h"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

using fmt::format;
using phosphor::logging::level;
using phosphor::logging::log;

std::string Serializer::Serialize(const NemoraDatagram* dgram)
{
    std::string result;
    switch (dgram->type)
    {
        case NemoraDatagramType::NemoraEvent:
            result = SerializeEvent(static_cast<const NemoraEvent*>(dgram));
            break;
        default:
            log<level::ERR>(
                format("Type with ID {} not supported by "
                       "Serializer::Serialize(const NemoraDatagram*)",
                       static_cast<int>(dgram->type))
                    .c_str());
    }

    return result;
}

std::string Serializer::SerializeEvent(const NemoraEvent* event)
{
    std::string result;
    platforms::nemora::proto::EventSeries pb;

    pb.set_magic(NEMORA_EVENT_PB_MAGIC);

    const char* p_arr = reinterpret_cast<const char*>(event->mac);
    pb.set_mac(p_arr, MAC_ADDR_SIZE);

    pb.set_sent_time_us(event->sent_time_s * 1000000);

    for (auto postcode : event->postcodes)
    {
        pb.add_postcodes(postcode);
    }

    pb.set_postcodes_protocol(
        platforms::nemora::proto::EventSeries::NATIVE_32_BIT);

    log<level::INFO>(
        format("NemoraEvent {}", pb.DebugString().c_str()).c_str());
    pb.SerializeToString(&result);
    return result;
}
