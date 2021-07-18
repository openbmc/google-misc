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
