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

#include <fcntl.h>
#include <fmt/format.h>
#include <libcr51sign/cr51_image_descriptor.h>
#include <libcr51sign/cr51sign_validator.h>
#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_support.h>
#include <sys/types.h>
#include <unistd.h>

#include <flashupdate/cr51.hpp>
#include <flashupdate/logging.hpp>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace flashupdate
{
namespace cr51
{

std::string formatImageVersion(const struct image_descriptor& descriptor)
{
    return fmt::format("{}.{}.{}", descriptor.image_major,
                       descriptor.image_minor, descriptor.image_point);
}

bool Cr51::readCr51(bool prod)
{
    log(LogLevel::Info, "INFO: Read CR51 Descriptor for {} with size of {}\n",
        file, size);

    context.start_offset = 0;
    context.end_offset = static_cast<uint32_t>(size);
    context.current_image_family = IMAGE_FAMILY_ALL;
    context.current_image_type = IMAGE_PROD;
    context.keyring_len = kSignatureRsa4096Pkcs15KeyLength;
    context.keyring = prod ? keys.prod.data() : keys.dev.data();
    context.priv = &shaContext;

    if (google::cr51::readImage(file.c_str(), size))
    {
        log(LogLevel::Error, "ERROR: Failed to read BIOS image\n");

        return false;
    }

    log(LogLevel::Info, "INFO: Finishing reading BIOS image\n");

    return true;
}

std::string Cr51::imageVersion()
{
    return version;
}

bool Cr51::verify(bool prod)
{
    // Disabled stdout for all info messages
    fpos_t pos;
    // Save stdout location
    log(LogLevel::Info, "INFO: Disable STDOUT\n");

    fgetpos(stdout, &pos);
    int fd = dup(fileno(stdout));

    // Redirect them to /dev/null
    auto e = freopen("/dev/null", "w", stdout);
    if (!e)
    {
        throw std::runtime_error("failed to redirect stdout to /dev/null");
    }
    readCr51(prod);

    struct libcr51sign_validated_regions imageRegions;

    auto maybeImageRegions = google::cr51::validateDescriptor(&context);
    if (!maybeImageRegions)
    {
        log(LogLevel::Crit, "cr51 sign is invalid for {}\n", file.data());
        return false;
    }
    imageRegions = *maybeImageRegions;

    log(LogLevel::Info, "INFO: Passed CR51 Sign Validation\n");

    regions = std::vector<struct image_region>(&imageRegions.image_regions[0],
                                               &imageRegions.image_regions[0] +
                                                   imageRegions.region_count);
    version = formatImageVersion(context.descriptor);
    log(LogLevel::Info, "INFO: BIOS Version: {}\n", version);

    fflush(stdout);
    dup2(fd, fileno(stdout)); // restore the stdout
    close(fd);
    clearerr(stdout);

    // Move stdout to the normal location
    fsetpos(stdout, &pos);

    log(LogLevel::Info, "INFO: Enable STDOUT\n");

    // Create the HASH of the CR51 descriptor on the BIOS
    auto maybeHash = google::cr51::hashDescriptor(&context);
    if (maybeHash)
    {
        hash = *maybeHash;
    }

    // Update the sign type for the BIOS image
    this->prod = prod;

    return true;
}

std::vector<struct image_region> Cr51::persistentRegions()
{
    if (!valid)
        return {};

    std::vector<struct image_region> persistentRegion;

    for (const auto& region : regions)
    {
        if ((region.region_attributes &
             (IMAGE_REGION_PERSISTENT | IMAGE_REGION_PERSISTENT_RELOCATABLE |
              IMAGE_REGION_PERSISTENT_EXPANDABLE)) > 0)
        {
            persistentRegion.emplace_back(region);
        };
    }

    return persistentRegion;
}

} // namespace cr51
} // namespace flashupdate