#pragma once

#include <blobs-ipmid/blobs.hpp>
#include <metric.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace blobs
{

class MetricBlobHandler : public GenericBlobInterface
{
  public:
    MetricBlobHandler() = default;
    ~MetricBlobHandler() = default;
    MetricBlobHandler(const MetricBlobHandler&) = delete;
    MetricBlobHandler& operator=(const MetricBlobHandler&) = delete;
    MetricBlobHandler(MetricBlobHandler&&) = default;
    MetricBlobHandler& operator=(MetricBlobHandler&&) = default;

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
    bool isReadOnlyOpenFlags(const uint16_t flag);
    /* Every session gets its own BmcHealthSnapshot instance. */
    std::unordered_map<uint16_t,
                       std::unique_ptr<metric_blob::BmcHealthSnapshot>>
        sessions;
};

} // namespace blobs
