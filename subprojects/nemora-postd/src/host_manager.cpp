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

#include "host_manager.hpp"

#include <fmt/format.h>

#include <functional>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <variant>

using fmt::format;
using phosphor::logging::level;
using phosphor::logging::log;

HostManager::HostManager() :
    postcodes_(), bus_(sdbusplus::bus::new_default()),
    signal_(bus_, HostManager::GetMatch().c_str(),
            [this](auto& m) -> void { this->DbusHandleSignal(m); }),
    post_poller_enabled_(true)
{
    // Spin off thread to listen on bus_
    auto post_poller_thread = std::mem_fn(&HostManager::PostPollerThread);
    post_poller_ = std::make_unique<std::thread>(post_poller_thread, this);
}

int HostManager::DbusHandleSignal(sdbusplus::message::message& msg)
{
    log<level::INFO>("Property Changed!");
    std::string msgSensor, busName{POSTCODE_BUSNAME};
    std::map<std::string, std::variant<std::tuple<uint64_t, std::vector<uint8_t>>>> msgData;
    msg.read(msgSensor, msgData);

    if (msgSensor == busName)
    {
        auto valPropMap = msgData.find("Value");
        if (valPropMap != msgData.end())
        {
            uint64_t rawValue = std::get<uint64_t>(std::get<0>(valPropMap->second));

            PushPostcode(rawValue);
        }
    }

    return 0;
}

void HostManager::PushPostcode(uint64_t postcode)
{
    // Get lock
    std::lock_guard<std::mutex> lock(postcodes_lock_);
    // Add postcode to queue
    postcodes_.push_back(postcode);
}

std::vector<uint64_t> HostManager::DrainPostcodes()
{
    // Get lock
    std::lock_guard<std::mutex> lock(postcodes_lock_);

    auto count = postcodes_.size();
    if (count > 0)
    {
        std::string msg = format("Draining Postcodes. Count: {}.", count);
        log<level::ERR>(msg.c_str());
    }

    // Drain the queue into a list
    // TODO: maximum # postcodes?
    std::vector<uint64_t> result(postcodes_);
    postcodes_.clear();

    return result;
}

std::string HostManager::GetMatch()
{
    std::string obj{POSTCODE_OBJECTPATH};
    return std::string("type='signal',"
                       "interface='org.freedesktop.DBus.Properties',"
                       "member='PropertiesChanged',"
                       "path='" +
                       obj + "'");
}

void HostManager::PostPollerThread()
{
    while (post_poller_enabled_)
    {
        bus_.process_discard();
        bus_.wait();
    }
}
