// Copyright 2022 Google LLC
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

#include "blobHandler.hpp"

#include <hostboottime.pb.h>

#include <btDefinitions.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

constexpr const bool debug = true;

namespace blobs
{

bool BlobHandler::canHandleBlob(const std::string& path)
{
    return path == kBTBlobPath;
}

// A blob handler may have multiple Blobs. For this blob handler, there is only
// one blob.
std::vector<std::string> BlobHandler::getBlobIds()
{
    return {std::string(kBTBlobPath)};
}

// BmcBlobDelete (7) is not supported.
bool BlobHandler::deleteBlob([[maybe_unused]] const std::string& path)
{
    return false;
}

// BmcBlobStat (8) (global stat) is not supported.
bool BlobHandler::stat([[maybe_unused]] const std::string& path,
                       [[maybe_unused]] BlobMeta* meta)
{
    return false;
}

// BmcBlobOpen(2) handler.
bool BlobHandler::open(uint16_t session, uint16_t flags,
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

    nlohmann::json j;
    std::ifstream fin(kFinalJson);
    if (!fin.is_open())
    {
        std::cerr << "[WARNING]: Boot time data not found." << std::endl;
        return false;
    }
    fin >> j;

    // Durations
    host::boottimeproto::HostBootDuration duration;
    duration.set_osuserdown(
        j[BTCategory::kDuration].contains(BTDuration::kOSUserDown)
            ? j[BTCategory::kDuration][BTDuration::kOSUserDown].get<uint64_t>()
            : 0);
    duration.set_oskerneldown(
        j[BTCategory::kDuration].contains(BTDuration::kOSKernelDown)
            ? j[BTCategory::kDuration][BTDuration::kOSKernelDown]
                  .get<uint64_t>()
            : 0);
    duration.set_bmcdown(
        j[BTCategory::kDuration].contains(BTDuration::kBMCDown)
            ? j[BTCategory::kDuration][BTDuration::kBMCDown].get<uint64_t>()
            : 0);
    duration.set_bmc(
        j[BTCategory::kDuration].contains(BTDuration::kBMC)
            ? j[BTCategory::kDuration][BTDuration::kBMC].get<uint64_t>()
            : 0);
    duration.set_bios(
        j[BTCategory::kDuration].contains(BTDuration::kBIOS)
            ? j[BTCategory::kDuration][BTDuration::kBIOS].get<uint64_t>()
            : 0);
    duration.set_nerfkernel(
        j[BTCategory::kDuration].contains(BTDuration::kNerfKernel)
            ? j[BTCategory::kDuration][BTDuration::kNerfKernel].get<uint64_t>()
            : 0);
    duration.set_nerfuser(
        j[BTCategory::kDuration].contains(BTDuration::kNerfUser)
            ? j[BTCategory::kDuration][BTDuration::kNerfUser].get<uint64_t>()
            : 0);
    duration.set_oskernel(
        j[BTCategory::kDuration].contains(BTDuration::kOSKernel)
            ? j[BTCategory::kDuration][BTDuration::kOSKernel].get<uint64_t>()
            : 0);
    duration.set_osuser(
        j[BTCategory::kDuration].contains(BTDuration::kOSUser)
            ? j[BTCategory::kDuration][BTDuration::kOSUser].get<uint64_t>()
            : 0);
    duration.set_unmeasured(
        j[BTCategory::kDuration].contains(BTDuration::kUnmeasured)
            ? j[BTCategory::kDuration][BTDuration::kUnmeasured].get<uint64_t>()
            : 0);
    duration.set_total(
        j[BTCategory::kDuration].contains(BTDuration::kTotal)
            ? j[BTCategory::kDuration][BTDuration::kTotal].get<uint64_t>()
            : 0);
    if (j[BTCategory::kDuration].contains(BTDuration::kExtra))
    {
        for (auto& el : j[BTCategory::kDuration][BTDuration::kExtra].items())
        {
            auto extra = duration.add_extra();
            extra->set_name(el.key());
            extra->set_milliseconds(el.value());
        }
    }

    // Statistic
    host::boottimeproto::HostBootStatistic statistic;
    statistic.set_internalrebootcount(
        j[BTCategory::kStatistic].contains(BTStatistic::kInternalRebootCount)
            ? j[BTCategory::kStatistic][BTStatistic::kInternalRebootCount]
                  .get<int32_t>()
            : 0);
    statistic.set_powercycletype(
        j[BTCategory::kStatistic].contains(BTStatistic::kIsACPowerCycle)
            ? j[BTCategory::kStatistic][BTStatistic::kIsACPowerCycle]
                  .get<bool>()
            : true);

    // HostBootTimeInfo
    host::boottimeproto::HostBootTimeInfo btInfo;
    *(btInfo.mutable_durations()) = duration;
    *(btInfo.mutable_statistics()) = statistic;

    size_t size = btInfo.ByteSizeLong();
    std::vector<char> vec(size);

    if (!btInfo.SerializeToArray(vec.data(), size))
    {
        std::cerr << "[ERROR]: Could not serialize protobuf to array"
                  << std::endl;
    }

    if (debug)
    {
        std::stringstream ss;
        ss << "[DEBUG]: Serialized Size = " << size << std::endl;
        ss << "          00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f"
           << std::endl;
        constexpr const size_t kRowSize = 16;

        for (size_t i = 0; i < size; i += kRowSize)
        {
            std::string asciiStr = "";
            ss << std::setfill('0') << std::setw(8) << std::hex << i << "  ";

            const uint8_t kCurRowSize = std::min(kRowSize, vec.size() - i);
            for (uint8_t j = 0; j < kCurRowSize; j++)
            {
                ss << std::setfill('0') << std::setw(2) << std::hex
                   << (static_cast<uint16_t>(vec[i * kRowSize + j]) & 0xff)
                   << " ";
                if (j == 7)
                {
                    ss << ' ';
                }

                asciiStr.push_back(std::isprint(vec[i * kRowSize + j])
                                       ? vec[i * kRowSize + j]
                                       : '.');
            }

            // need one more space if (kCurRowSize < 8) becasue 1 space will be
            // appended behind 8th char.
            uint32_t neededSpace =
                1 + (kRowSize - kCurRowSize) * 3 + (kCurRowSize < 8 ? 1 : 0);
            while (neededSpace--)
            {
                ss << " ";
            }
            ss << "|" << asciiStr << "|" << std::endl;
        }
        std::cerr << ss.str();
    }

    sessions[session] = std::move(vec);
    return true;
}

// BmcBlobRead(3) handler.
std::vector<uint8_t> BlobHandler::read(uint16_t session, uint32_t offset,
                                       uint32_t requestedSize)
{
    auto it = sessions.find(session);
    if (it == sessions.end())
    {
        return {};
    }

    return std::vector<uint8_t>(
        it->second.data() + offset,
        it->second.data() + offset +
            std::min(offset + requestedSize,
                     static_cast<uint32_t>(it->second.size())));
}

// BmcBlobWrite(4) is not supported.
bool BlobHandler::write([[maybe_unused]] uint16_t session,
                        [[maybe_unused]] uint32_t offset,
                        [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobWriteMeta(10) is not supported.
bool BlobHandler::writeMeta([[maybe_unused]] uint16_t session,
                            [[maybe_unused]] uint32_t offset,
                            [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobCommit(5) is not supported.
bool BlobHandler::commit([[maybe_unused]] uint16_t session,
                         [[maybe_unused]] const std::vector<uint8_t>& data)
{
    return false;
}

// BmcBlobClose(6) handler.
bool BlobHandler::close(uint16_t session)
{
    auto itr = sessions.find(session);
    if (itr == sessions.end())
    {
        return false;
    }
    sessions.erase(itr);
    return true;
}

bool BlobHandler::stat(uint16_t session, BlobMeta* meta)
{
    auto it = sessions.find(session);
    if (it == sessions.end())
    {
        return false;
    }

    meta->blobState = 0;
    meta->blobState = blobs::StateFlags::open_read;
    meta->size = it->second.size();

    return true;
}

bool BlobHandler::expire(uint16_t session)
{
    return close(session);
}

// Checks for a read-only flag.
bool BlobHandler::isReadOnlyOpenFlags(const uint16_t flags)
{
    if (((flags & blobs::OpenFlags::read) == blobs::OpenFlags::read) &&
        ((flags & blobs::OpenFlags::write) == 0))
    {
        return true;
    }
    return false;
}

} // namespace blobs
