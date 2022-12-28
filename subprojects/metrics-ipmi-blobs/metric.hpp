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
    bmcmetrics::metricproto::BmcDaemonStatMetric getDaemonStatList();

    int getStringID(const std::string_view s);
    std::atomic<bool> done;
    std::vector<char> pbDump;
    std::unordered_map<std::string, int> stringTable;
    int stringId;
    long ticksPerSec;
};

} // namespace metric_blob
