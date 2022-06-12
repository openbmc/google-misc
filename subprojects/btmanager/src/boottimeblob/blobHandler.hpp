// Copyright 2022 Google LLC
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

#include <blobs-ipmid/blobs.hpp>
#include <hostboottime.pb.h>
#include <cstdint>
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

namespace blobs
{
    // This blob is a read-only blob can only handle single blob id.
    class BlobHandler : public GenericBlobInterface
    {
    public:
        constexpr const static char* kBTBlobPath = "/host/boottime";

        BlobHandler() = default;

        bool canHandleBlob(const std::string& path) override;
        std::vector<std::string> getBlobIds() override;
        bool deleteBlob(const std::string& path) override;
        bool stat(const std::string& path, BlobMeta* meta) override;
        bool open(uint16_t session, uint16_t flags, const std::string& path) override;
        std::vector<uint8_t> read(uint16_t session, uint32_t offset, uint32_t requestedSize) override;
        bool write(uint16_t session, uint32_t offset, const std::vector<uint8_t>& data) override;
        bool writeMeta(uint16_t session, uint32_t offset, const std::vector<uint8_t>& data) override;
        bool commit(uint16_t session, const std::vector<uint8_t>& data) override;
        bool close(uint16_t session) override;
        bool stat(uint16_t session, BlobMeta* meta) override;
        bool expire(uint16_t session) override;

    private:
        bool isReadOnlyOpenFlags(const uint16_t flag);
        std::unordered_map<uint16_t, uint16_t/*protobuf_struct*/> sessions;
    };
}
