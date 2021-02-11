#pragma once

#include "metricblob.pb.h"

#include <unistd.h>

#include <blobs-ipmid/blobs.hpp>

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace metric_blob
{

class BmcHealthSnapshot
{
  public:
    BmcHealthSnapshot();

    /**
     * Reads data from this metric
     * @param offset: offset into the data to read
     * @param requestedSize: how many bytes to read
     * @returns Bytes able to read. Returns empty if nothing can be read.
     */
    std::string_view read(uint32_t offset, uint32_t requestedSize);

    /**
     * Returns information about the amount of readable data and whether the
     * metric has finished populating.
     * @param meta: Struct to fill with the metadata info
     */
    bool stat(blobs::BlobMeta& meta);

    /**
     * Start the metric collection process
     */
    void doWork();

    /**
     * The size of the content string.
     */
    uint32_t size();

  private:
    /**
     * Serialize to the pb_dump_ array.
     */
    void serializeSnapshotToArray(
        const bmcmetrics::metricproto::BmcMetricSnapshot& snapshot);

    // The two following functions access the snapshot's string table so they
    // have to be member functions.
    bmcmetrics::metricproto::BmcProcStatMetric getProcStatList();
    bmcmetrics::metricproto::BmcFdStatMetric getFdStatList();

    int getStringID(const std::string_view s);
    std::atomic<bool> done;
    std::vector<char> pbDump;
    std::unordered_map<std::string, int> stringTable;
    int stringId;
    long ticksPerSec;
};

} // namespace metric_blob
