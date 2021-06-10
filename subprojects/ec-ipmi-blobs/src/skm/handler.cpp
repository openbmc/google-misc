#include <fmt/format.h>

#include <ec-ipmi-blobs/skm/cmd.hpp>
#include <ec-ipmi-blobs/skm/handler.hpp>
#include <ipmid/api-types.hpp>
#include <ipmid/handler.hpp>

#include <charconv>
#include <string_view>
#include <utility>

namespace blobs
{
namespace ec
{
namespace skm
{

Handler::Handler(Cmd& cmd) : cmd(&cmd)
{}

constexpr std::string_view blobPrefix = "/skm/hss/";

std::optional<uint8_t> blobToIdx(std::string_view path) noexcept
{
    if (path.substr(0, blobPrefix.size()) != blobPrefix)
    {
        return std::nullopt;
    }
    uint8_t skm_idx;
    auto res = std::from_chars(path.data() + blobPrefix.size(),
                               path.data() + path.size(), skm_idx);
    if (std::make_error_code(res.ec) || res.ptr != path.data() + path.size() ||
        skm_idx >= numKeys)
    {
        return std::nullopt;
    }
    return skm_idx;
}

bool Handler::canHandleBlob(const std::string& path)
{
    return blobToIdx(path).has_value();
}

std::vector<std::string> Handler::getBlobIds()
{
    std::vector<std::string> ret(numKeys);
    for (size_t i = 0; i < numKeys; ++i)
        ret[i] = fmt::format("{}{}", blobPrefix, i);
    return ret;
}

bool Handler::deleteBlob(const std::string&)
{
    throw ipmi::HandlerCompletion(ipmi::ccIllegalCommand);
}

bool Handler::stat(const std::string&, BlobMeta* meta)
{
    meta->blobState = 0;
    meta->size = keySize;
    return true;
}

static void setCommit(uint32_t& state, uint32_t commit_state) noexcept
{
    state &= ~(committed | committing | commit_error);
    state |= commit_state;
}

bool Handler::open(uint16_t session, uint16_t flags, const std::string& path)
{
    uint8_t skm_idx = *blobToIdx(path);
    if (sessions[skm_idx])
    {
        throw ipmi::HandlerCompletion(ipmi::ccDuplicateRequest);
    }
    auto& s = sessions[skm_idx].emplace();
    sessionMap[session] = skm_idx;
    if (flags & OpenFlags::read)
    {
        auto cb = [&s, flags](uint8_t res, stdplus::span<std::byte> key) {
            if (res == 0)
            {
                s.blobState = committed | open_read |
                              (flags & OpenFlags::write ? open_write : 0);
                std::memcpy(s.key.data(), key.data(), keySize);
            }
            else
            {
                s.blobState = commit_error;
            }
            s.outstanding.reset();
        };
        s.outstanding = readKey(*cmd, skm_idx, std::move(cb));
        s.blobState = committing;
    }
    else
    {
        s.blobState |= flags & OpenFlags::write ? open_write : 0;
        s.key = {};
    }
    return true;
}

std::vector<uint8_t> Handler::read(uint16_t session, uint32_t offset,
                                   uint32_t requestedSize)
{
    auto& s = sessionFromId(session);
    if (!(s.blobState & open_read))
    {
        throw ipmi::HandlerCompletion(ipmi::ccIllegalCommand);
    }
    if (offset > s.key.size())
    {
        throw ipmi::HandlerCompletion(ipmi::ccReqDataLenInvalid);
    }
    std::vector<uint8_t> ret(
        std::min<size_t>(s.key.size() - offset, requestedSize));
    std::memcpy(ret.data(), s.key.data() + offset, ret.size());
    return ret;
}

bool Handler::write(uint16_t session, uint32_t offset,
                    const std::vector<uint8_t>& data)
{
    auto& s = sessionFromId(session);
    if (!(s.blobState & open_write))
    {
        throw ipmi::HandlerCompletion(ipmi::ccIllegalCommand);
    }
    if (offset > s.key.size())
    {
        throw ipmi::HandlerCompletion(ipmi::ccReqDataLenInvalid);
    }
    auto start = s.key.data() + offset;
    auto size = std::min<size_t>(s.key.size() - offset, data.size());
    if (!(s.blobState & committed) || std::memcmp(start, data.data(), size))
    {
        std::memcpy(start, data.data(), size);
        s.blobState &= ~committed;
    }
    if (offset + data.size() > s.key.size())
    {
        throw ipmi::HandlerCompletion(ipmi::ccReqDataTruncated);
    }
    return true;
}

bool Handler::writeMeta(uint16_t, uint32_t, const std::vector<uint8_t>&)
{
    throw ipmi::HandlerCompletion(ipmi::ccIllegalCommand);
}

bool Handler::commit(uint16_t session, const std::vector<uint8_t>&)
{
    const auto it = sessionToIt(session);
    auto& s = *sessions[it->second];
    if (s.outstanding)
    {
        return true;
    }
    if (s.blobState & committed)
    {
        return true;
    }
    s.outstanding = writeKey(*cmd, it->second, s.key, [&s](uint8_t res) {
        setCommit(s.blobState, res == 0 ? committed : commit_error);
        s.outstanding.reset();
    });
    setCommit(s.blobState, committing);
    return true;
}

bool Handler::close(uint16_t session)
{
    const auto it = sessionToIt(session);
    sessions[it->second].reset();
    sessionMap.erase(it);
    return true;
}

bool Handler::stat(uint16_t session, BlobMeta* meta)
{
    meta->blobState = sessionFromId(session).blobState;
    meta->size = keySize;
    return true;
}

bool Handler::expire(uint16_t session)
{
    try
    {
        close(session);
    }
    catch (...)
    {}
    return true;
}

std::map<uint16_t, Handler::KeyId>::iterator
    Handler::sessionToIt(uint16_t session)
{
    const auto it = sessionMap.find(session);
    if (it == sessionMap.end())
    {
        throw ipmi::HandlerCompletion(ipmi::ccInvalidReservationId);
    }
    return it;
}

Handler::Session& Handler::sessionFromId(uint16_t session)
{
    return *sessions[sessionToIt(session)->second];
}

} // namespace skm
} // namespace ec
} // namespace blobs
