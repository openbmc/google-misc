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
#include "serializer.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

class SocketManager
{
  public:
    SocketManager() = default;
    ~SocketManager();

    /**
     * Sends a UDP packet to the address named in bcast object.
     */
    void SendDatagram(const NemoraDatagram* bcast);

    /**
     * Checks content of open_sockets_ and closes the socket if it is contained
     * in the list. Closing a socket which is already closed causes problems.
     */
    void CloseSocketSafely(int fd);

  private:
    /**
     * Adds a socket fd to open_sockets_ to allow tracking of which sockets are
     * open or not.  Closing a socket which is already closed causes problems.
     */
    void TrackSocket(int fd);
    std::unordered_set<int> open_sockets_;
    std::mutex open_sockets_lock_;
};
