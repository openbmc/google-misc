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

#include "dbus.hpp"

#include <blobs-ipmid/blobs.hpp>
#include <stdplus/cancel.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ipmi_hoth
{

struct HothBlob
{
    HothBlob(uint16_t id, std::string&& hothId, uint16_t flags,
             uint32_t maxBufferSize) :
        sessionId(id),
        hothId(std::move(hothId)), state(0)
    {
        if (flags & blobs::OpenFlags::read)
        {
            state |= blobs::StateFlags::open_read;
        }
        if (flags & blobs::OpenFlags::write)
        {
            state |= blobs::StateFlags::open_write;
        }

        /* Pre-allocate the buffer.capacity() with maxBufferSize */
        buffer.reserve(maxBufferSize);
    }
    ~HothBlob()
    {
        /* We want to deliberately wipe the buffer to avoid leaking any
         * sensitive ProdID secrets.
         */
        buffer.assign(buffer.capacity(), 0);
    }

    /* The blob handler session id. */
    uint16_t sessionId;

    /* The identifier for the hoth */
    std::string hothId;

    /* The current state. */
    uint16_t state;

    /* The staging buffer. */
    std::vector<uint8_t> buffer;

    /* Outstanding async operation */
    stdplus::Cancel outstanding;
};

class HothBlobHandler : public blobs::GenericBlobInterface
{
  public:
    static constexpr std::string_view pathPrefix = "/dev/hoth/";

    bool canHandleBlob(const std::string& path) override;
    std::vector<std::string> getBlobIds() override;
    bool deleteBlob(const std::string& path) override;
    virtual bool stat(const std::string& path, blobs::BlobMeta* meta) = 0;
    bool open(uint16_t session, uint16_t flags,
              const std::string& path) override;
    std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                              uint32_t requestedSize) override;
    bool write(uint16_t session, uint32_t offset,
               const std::vector<uint8_t>& data) override;
    bool writeMeta(uint16_t session, uint32_t offset,
                   const std::vector<uint8_t>& data) override;
    virtual bool commit(uint16_t session, const std::vector<uint8_t>& data) = 0;
    bool close(uint16_t session) override;
    virtual bool stat(uint16_t session, blobs::BlobMeta* meta) = 0;
    bool expire(uint16_t session) override;

    virtual internal::Dbus& dbus() = 0;
    virtual std::string_view pathSuffix() const = 0;
    virtual uint16_t requiredFlags() const = 0;
    virtual uint16_t maxSessions() const = 0;
    virtual uint32_t maxBufferSize() const = 0;

    /** @brief Takes a valid hoth blob path and turns it into a hoth id
     *
     *  @param[in] path - Hoth blob path to parse
     *  @return The hoth id represented by the path
     */
    std::string_view pathToHothId(std::string_view path);

    /** @brief Takes a hoth id and turns it into a fully qualified path for
     *         the current hoth handler.
     *
     *  @param[in] hothId - The hoth id
     *  @return The fully qualified blob path
     */
    std::string hothIdToPath(std::string_view hothId);

  protected:
    HothBlob* getSession(uint16_t id);
    std::optional<uint16_t> getOnlySession(std::string_view hothId);

  private:
    std::unordered_map<std::string, std::unordered_set<uint16_t>> pathSessions;
    std::unordered_map<uint16_t, std::unique_ptr<HothBlob>> sessions;
};

} // namespace ipmi_hoth
