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

#include "hoth.hpp"

#include <ipmid/api-types.hpp>
#include <ipmid/handler.hpp>
#include <stdplus/util/string.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ipmi_hoth
{

std::string_view HothBlobHandler::pathToHothId(std::string_view path)
{
    path.remove_prefix(pathPrefix.size());
    auto sep = path.find(pathSuffix());
    if (sep == 0)
    {
        return "";
    }
    return path.substr(0, sep - 1);
}

std::string HothBlobHandler::hothIdToPath(std::string_view hothId)
{
    if (hothId.empty())
    {
        return stdplus::util::strCat(pathPrefix, pathSuffix());
    }
    return stdplus::util::strCat(pathPrefix, hothId, "/", pathSuffix());
}

HothBlob* HothBlobHandler::getSession(uint16_t id)
{
    auto search = sessions.find(id);
    if (search == sessions.end())
    {
        return nullptr;
    }

    /* Not thread-safe, however, the blob handler deliberately assumes serial
     * execution. */
    return search->second.get();
}

std::optional<uint16_t> HothBlobHandler::getOnlySession(std::string_view hothId)
{
    /* This method is only valid if we only allow 1 session */
    if (maxSessions() != 1)
    {
        return std::nullopt;
    }

    for (auto& session : pathSessions[std::string(hothId)])
    {
        return session;
    }

    return std::nullopt;
}

bool HothBlobHandler::canHandleBlob(const std::string& path)
{
    std::string_view path_view(path);

    // Process the prefix element
    if (path_view.substr(0, pathPrefix.size()) != pathPrefix)
    {
        return false;
    }
    path_view.remove_prefix(pathPrefix.size());

    // Remove the identifier if specified
    auto sep = path_view.find('/');
    if (sep != std::string_view::npos)
    {
        path_view.remove_prefix(sep + 1);
    }

    return path_view == pathSuffix();
}

std::vector<std::string> HothBlobHandler::getBlobIds()
{
    std::vector<std::string> ret;
    for (const auto& obj : dbus().getHothdMapping())
    {
        std::string_view objView = obj.first;
        std::string_view objPrefix = "/xyz/openbmc_project/Control/Hoth";
        if (objView.substr(0, objPrefix.size()) != objPrefix)
        {
            continue;
        }
        objView.remove_prefix(objPrefix.size());
        if (objView.empty())
        {
            ret.push_back(hothIdToPath(""));
            continue;
        }
        if (objView[0] != '/')
        {
            continue;
        }
        objView.remove_prefix(1);
        auto sep = objView.find('/');
        if (sep != std::string_view::npos)
        {
            continue;
        }
        ret.push_back(hothIdToPath(objView));
    }
    return ret;
}

bool HothBlobHandler::deleteBlob(const std::string&)
{
    /* Hoth blob handler does not support a blob delete. */
    return false;
}

bool HothBlobHandler::open(uint16_t session, uint16_t flags,
                           const std::string& path)
{
    /* We require both flags set. */
    if ((flags & requiredFlags()) != requiredFlags())
    {
        /* Both flags not set. */
        return false;
    }

    auto findSess = sessions.find(session);
    if (findSess != sessions.end())
    {
        /* This session is already active. */
        return false;
    }

    auto hothId = std::string(pathToHothId(path));
    auto pathSession = pathSessions.find(hothId);
    if (pathSession != pathSessions.end() &&
        pathSession->second.size() >= maxSessions())
    {
        return false;
    }

    // Prevent host from adding lots of bad entries to the table by verifying
    // the hoth exists.
    if (pathSession == pathSessions.end() && !dbus().pingHothd(hothId))
    {
        return false;
    }

    pathSessions[hothId].emplace(session);
    sessions.emplace(session,
                     std::make_unique<HothBlob>(session, std::move(hothId),
                                                flags, maxBufferSize()));
    return true;
}

std::vector<uint8_t> HothBlobHandler::read(uint16_t session, uint32_t offset,
                                           uint32_t requestedSize)
{
    HothBlob* sess = getSession(session);
    if (!sess || !(sess->state & blobs::StateFlags::open_read) ||
        offset > sess->buffer.size())
    {
        throw ipmi::HandlerCompletion(ipmi::ccUnspecifiedError);
    }
    std::vector<uint8_t> ret(
        std::min<size_t>(requestedSize, sess->buffer.size() - offset));
    std::memcpy(ret.data(), sess->buffer.data() + offset, ret.size());
    return ret;
}

bool HothBlobHandler::write(uint16_t session, uint32_t offset,
                            const std::vector<uint8_t>& data)
{
    uint32_t newBufferSize = data.size() + offset;
    HothBlob* sess = getSession(session);
    if (!sess || !(sess->state & blobs::StateFlags::open_write) ||
        newBufferSize > maxBufferSize())
    {
        return false;
    }

    /* Resize the buffer if what we're writing will go over the size */
    if (newBufferSize > sess->buffer.size())
    {
        sess->buffer.resize(newBufferSize);
        sess->state &= ~blobs::StateFlags::committed;
    }

    /* Clear the comitted bit if our data isn't identical to existing data */
    if (std::memcmp(sess->buffer.data() + offset, data.data(), data.size()))
    {
        sess->state &= ~blobs::StateFlags::committed;
    }
    std::memcpy(sess->buffer.data() + offset, data.data(), data.size());
    return true;
}

bool HothBlobHandler::writeMeta(uint16_t, uint32_t, const std::vector<uint8_t>&)
{
    /* Hoth blob handler does not support meta write. */
    return false;
}

bool HothBlobHandler::close(uint16_t session)
{
    auto session_it = sessions.find(session);
    if (session_it == sessions.end())
    {
        return false;
    }

    auto path_it = pathSessions.find(session_it->second->hothId);
    path_it->second.erase(session);
    if (path_it->second.empty())
    {
        pathSessions.erase(path_it);
    }

    sessions.erase(session_it);
    return true;
}

bool HothBlobHandler::expire(uint16_t session)
{
    return close(session);
}

} // namespace ipmi_hoth
