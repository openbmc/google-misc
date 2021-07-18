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

#include "dbus_command.hpp"
#include "hoth.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ipmi_hoth
{

class HothCommandBlobHandler : public HothBlobHandler
{
  public:
    explicit HothCommandBlobHandler(internal::DbusCommand* dbus) : dbus_(dbus)
    {}

    /* Our callbacks require pinned memory */
    HothCommandBlobHandler(HothCommandBlobHandler&&) = delete;
    HothCommandBlobHandler& operator=(HothCommandBlobHandler&&) = delete;

    bool stat(const std::string& path, blobs::BlobMeta* meta) override;
    bool commit(uint16_t session, const std::vector<uint8_t>& data) override;
    bool stat(uint16_t session, blobs::BlobMeta* meta) override;

    internal::Dbus& dbus() override
    {
        return *dbus_;
    }
    std::string_view pathSuffix() const override
    {
        return "command_passthru";
    }
    uint16_t requiredFlags() const override
    {
        return blobs::OpenFlags::read | blobs::OpenFlags::write;
    }
    uint16_t maxSessions() const override
    {
        return 10;
    }
    uint32_t maxBufferSize() const override
    {
        return 1024;
    }

  private:
    internal::DbusCommand* dbus_;
};

} // namespace ipmi_hoth
