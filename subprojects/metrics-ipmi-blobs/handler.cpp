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

#include "handler.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace blobs
{

namespace
{
constexpr std::string_view metricPath("/metric/snapshot");
} // namespace

bool MetricBlobHandler::canHandleBlob(const std::string& path)
{
    return path == metricPath;
}

// A blob handler may have multiple Blobs. For this blob handler, there is only
// one blob.
std::vector<std::string> MetricBlobHandler::getBlobIds()
{
    return {std::string(metricPath)};
}

// BmcBlobDelete (7) is not supported.
bool MetricBlobHandler::deleteBlob(const std::string&)
{
    return false;
}

// BmcBlobStat (8) (global stat) is not supported.
bool MetricBlobHandler::stat(const std::string&, BlobMeta*)
{
    return false;
}

// Checks for a read-only flag.
bool MetricBlobHandler::isReadOnlyOpenFlags(const uint16_t flags)
{
    if (((flags & blobs::OpenFlags::read) == blobs::OpenFlags::read) &&
        ((flags & blobs::OpenFlags::write) == 0))
    {
        return true;
    }
    return false;
}

// BmcBlobOpen(2) handler.
bool MetricBlobHandler::open(uint16_t session, uint16_t flags,
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
    if (path == metricPath)
    {
        std::unique_ptr<metric_blob::BmcHealthSnapshot> bhs =
            std::make_unique<metric_blob::BmcHealthSnapshot>();
        bhs.get()->doWork();
        sessions[session] = nullptr;
        sessions[session] = std::move(bhs);
        return true;
    }
    return false;
}

// BmcBlobRead(3) handler.
std::vector<uint8_t> MetricBlobHandler::read(uint16_t session, uint32_t offset,
                                             uint32_t requestedSize)
{
    auto it = sessions.find(session);
    if (it == sessions.end())
    {
        return {};
    }

    std::string_view result = it->second->read(offset, requestedSize);
    return std::vector<uint8_t>(result.begin(), result.end());
}

// BmcBlobWrite(4) is not supported.
bool MetricBlobHandler::write(uint16_t, uint32_t, const std::vector<uint8_t>&)
{
    return false;
}

// BmcBlobWriteMeta(10) is not supported.
bool MetricBlobHandler::writeMeta(uint16_t, uint32_t,
                                  const std::vector<uint8_t>&)
{
    return false;
}

// BmcBlobCommit(5) is not supported.
bool MetricBlobHandler::commit(uint16_t, const std::vector<uint8_t>&)
{
    return false;
}

// BmcBlobClose(6) handler.
bool MetricBlobHandler::close(uint16_t session)
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
bool MetricBlobHandler::stat(uint16_t session, BlobMeta* meta)
{
    auto it = sessions.find(session);
    if (it == sessions.end())
    {
        return false;
    }
    return it->second->stat(*meta);
}

bool MetricBlobHandler::expire(uint16_t session)
{
    return close(session);
}

} // namespace blobs
