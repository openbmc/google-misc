#pragma once
#include <blobs-ipmid/blobs.hpp>
#include <ec-ipmi-blobs/cmd.hpp>
#include <ec-ipmi-blobs/skm/cmd.hpp>

#include <array>
#include <map>
#include <optional>

namespace blobs
{
namespace ec
{
namespace skm
{

class Handler : public GenericBlobInterface
{
  public:
    explicit Handler(Cmd& cmd);
    ~Handler() = default;
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(Handler&&) = delete;

    bool canHandleBlob(const std::string& path) override;
    std::vector<std::string> getBlobIds() override;
    bool deleteBlob(const std::string& path) override;
    bool stat(const std::string& path, BlobMeta* meta) override;
    bool open(uint16_t session, uint16_t flags,
              const std::string& path) override;
    std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                              uint32_t requestedSize) override;
    bool write(uint16_t session, uint32_t offset,
               const std::vector<uint8_t>& data) override;
    bool writeMeta(uint16_t session, uint32_t offset,
                   const std::vector<uint8_t>& data) override;
    bool commit(uint16_t session, const std::vector<uint8_t>& data) override;
    bool close(uint16_t session) override;
    bool stat(uint16_t session, BlobMeta* meta) override;
    bool expire(uint16_t session) override;

  private:
    Cmd* cmd;

    struct Session
    {
        uint32_t blobState;
        std::array<std::byte, keySize> key;
        std::optional<Cancel> outstanding;
    };
    std::array<std::optional<Session>, numKeys> sessions;
    using KeyId = uint8_t;
    // Ensure the KeyId type can hold all our keys
    static_assert(numKeys <= 1 << (sizeof(KeyId) << 3));
    std::map<uint16_t, KeyId> sessionMap;

    std::map<uint16_t, KeyId>::iterator sessionToIt(uint16_t session);
    Session& sessionFromId(uint16_t session);
};

} // namespace skm
} // namespace ec
} // namespace blobs
