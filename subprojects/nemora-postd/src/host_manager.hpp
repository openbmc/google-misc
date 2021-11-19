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

#pragma once
#include "nemora_types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#define POSTCODE_OBJECTPATH "/xyz/openbmc_project/state/boot/raw0"
#define POSTCODE_BUSNAME "xyz.openbmc_project.State.Boot.Raw"

class HostManager
{
  public:
    HostManager();
    ~HostManager() = default;

    /**
     * Callback for POST code DBus listener
     * - msg: out-parameter for message received over DBus from callback
     *
     * - returns: error code or 0 for success
     */
    int DbusHandleSignal(sdbusplus::message_t& msg);

    /**
     * Helper to construct match string for callback registration for POST
     * listener
     * - returns: match string for use in registering callback
     */
    static std::string GetMatch();

    /**
     * Copies contents of POSTcode vector away to allow for sending via UDP
     * - returns: vector filled with current state of postcodes_
     */
    std::vector<uint64_t> DrainPostcodes();

    /**
     * Add POST code to vector, thread-safely
     * - postcode: POST code to add, typically only 8 or 16 bits wide
     */
    void PushPostcode(uint64_t postcode);

  private:
    /**
     * Business logic of thread listening to DBus for POST codes
     */
    void PostPollerThread();

    // It's important that postcodes_ be initialized before post_poller_!
    std::vector<uint64_t> postcodes_;
    std::mutex postcodes_lock_;

    sdbusplus::bus_t bus_;
    sdbusplus::bus::match_t signal_;
    std::unique_ptr<std::thread> post_poller_;
    bool post_poller_enabled_;
};
