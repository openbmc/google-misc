#include "hoth_command.hpp"

#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using phosphor::logging::entry;
using phosphor::logging::log;

using level = phosphor::logging::level;
using SdBusError = sdbusplus::exception::SdBusError;

namespace ipmi_hoth
{

bool HothCommandBlobHandler::stat(const std::string&, blobs::BlobMeta*)
{
    /* Hoth command handler does not support a global blob stat. */
    return false;
}

bool HothCommandBlobHandler::commit(uint16_t session,
                                     const std::vector<uint8_t>& data)
{
    if (!data.empty())
    {
        log<level::ERR>("Unexpected data provided to commit call");
        return false;
    }

    HothBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }

    // If commit is called multiple times, return the same result as last time
    if (sess->state &
        (blobs::StateFlags::committing | blobs::StateFlags::committed))
    {
        return true;
    }

    sess->state &= ~blobs::StateFlags::commit_error;
    sess->state |= blobs::StateFlags::committing;
    sess->outstanding = dbus_->SendHostCommand(
        sess->hothId, sess->buffer,
        [sess](std::optional<std::vector<uint8_t>> rsp) {
            auto outstanding = std::move(sess->outstanding);
            sess->state &= ~blobs::StateFlags::committing;
            if (!rsp)
            {
                sess->state |= blobs::StateFlags::commit_error;
                return;
            }
            sess->buffer = std::move(*rsp);
            sess->state |= blobs::StateFlags::committed;
        });
    return true;
}

bool HothCommandBlobHandler::stat(uint16_t session, blobs::BlobMeta* meta)
{
    HothBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }

    meta->size = sess->buffer.size();
    meta->blobState = sess->state;
    return true;
}

} // namespace ipmi_hoth
