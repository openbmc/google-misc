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

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "blobHandler.hpp"
#include "btStateMachine.hpp"

namespace blobs
{

bool BlobHandler::canHandleBlob(const std::string& path)
{
    return path == kBTBlobPath;
}

// A blob handler may have multiple Blobs. For this blob handler, there is only
// one blob.
std::vector<std::string> BlobHandler::getBlobIds()
{
    return {std::string(kBTBlobPath)};
}

// BmcBlobDelete (7) is not supported.
bool BlobHandler::deleteBlob([[maybe_unused]] const std::string& path)
{
    return false;
}

// BmcBlobStat (8) (global stat) is not supported.
bool BlobHandler::stat([[maybe_unused]] const std::string& path, [[maybe_unused]] BlobMeta* meta)
{
    return false;
}

// BmcBlobOpen(2) handler.
bool BlobHandler::open(uint16_t session, uint16_t flags,
                             const std::string& path)
{
    if (!isReadOnlyOpenFlags(flags))
    {
        return false;
    }
    if (!canHandleBlob(path))
    {
        return false;
    }
    // std::unique_ptr<metric_blob::BmcHealthSnapshot> bhs =
    //     std::make_unique<metric_blob::BmcHealthSnapshot>();
    // bhs.get()->doWork();
    // sessions[session] = nullptr;
    // sessions[session] = std::move(bhs);
    sessions[session] = 0; // remove
    return true;
}

// BmcBlobRead(3) handler.
std::vector<uint8_t> BlobHandler::read([[maybe_unused]] uint16_t session, [[maybe_unused]] uint32_t offset,
                                             [[maybe_unused]] uint32_t requestedSize)
{
    auto it = sessions.find(session);
    if (it == sessions.end())
    {
        return {};
    }

    // std::string_view result = it->second->read(offset, requestedSize);
    // return std::vector<uint8_t>(result.begin(), result.end());
    return {};
}

// BmcBlobWrite(4) is not supported.
bool BlobHandler::write([[maybe_unused]] uint16_t session, [[maybe_unused]] uint32_t offset, [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobWriteMeta(10) is not supported.
bool BlobHandler::writeMeta([[maybe_unused]] uint16_t session, [[maybe_unused]] uint32_t offset,
                                  [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobCommit(5) is not supported.
bool BlobHandler::commit([[maybe_unused]] uint16_t session, [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobClose(6) handler.
bool BlobHandler::close(uint16_t session)
{
    auto itr = sessions.find(session);
    if (itr == sessions.end())
    {
        return false;
    }
    sessions.erase(itr);
    return true;
}

// BmcBlobSessionStat(9) handler
bool BlobHandler::stat([[maybe_unused]] uint16_t session, [[maybe_unused]] BlobMeta* meta)
{
    // if (!done)
    // {
    //     // Bits 8~15 are blob-specific state flags.
    //     // For this blob, bit 8 is set when metric collection is still in
    //     // progress.
    //     meta.blobState |= (1 << 8);
    // }
    // else
    // {
    //     meta.blobState = 0;
    //     meta.blobState = blobs::StateFlags::open_read;
    //     meta.size = pbDump.size();
    // }
    // return true;

    return false;
}

bool BlobHandler::expire(uint16_t session)
{
    return close(session);
}

// Checks for a read-only flag.
bool BlobHandler::isReadOnlyOpenFlags(const uint16_t flags)
{
    if (((flags & blobs::OpenFlags::read) == blobs::OpenFlags::read) &&
        ((flags & blobs::OpenFlags::write) == 0))
    {
        return true;
    }
    return false;
}

} // namespace blobs
