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

#include "nic_mock.h"

#include "platforms/nemora/portable/ncsi.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace mock
{

bool NCSIFrame::parse_ethernet_frame(const ncsi_buf_t& ncsi_buf)
{
    std::memcpy(&dst_mac_, ncsi_buf.data, sizeof(dst_mac_));
    std::memcpy(&src_mac_, ncsi_buf.data + sizeof(dst_mac_), sizeof(src_mac_));
    // The constant defined in a way that assumes big-endian platform, so we are
    // just going to calculate it here properly.
    const uint8_t et_hi = *(ncsi_buf.data + 2 * sizeof(mac_addr_t));
    const uint8_t et_lo = *(ncsi_buf.data + 2 * sizeof(mac_addr_t) + 1);
    ethertype_ = (et_hi << 8) + et_lo;

    if (ethertype_ != NCSI_ETHERTYPE)
    {
        return false;
    }

    // This code parses the NC-SI command, according to spec and
    // as defined in platforms/nemora/portable/ncsi.h
    // It takes some shortcuts to only retrieve the data we are interested in,
    // such as using offsetof ot get to a particular field.
    control_packet_type_ =
        *(ncsi_buf.data + offsetof(ncsi_header_t, control_packet_type));
    channel_id_ = *(ncsi_buf.data + offsetof(ncsi_header_t, channel_id));

    size_t payload_offset = sizeof(ncsi_header_t);
    if (control_packet_type_ & NCSI_RESPONSE)
    {
        is_response_ = true;
        control_packet_type_ &= ~NCSI_RESPONSE;
        std::memcpy(&response_code_, ncsi_buf.data + payload_offset,
                    sizeof(response_code_));
        response_code_ = ntohs(response_code_);
        std::memcpy(&reason_code_,
                    ncsi_buf.data + payload_offset + sizeof(reason_code_),
                    sizeof(reason_code_));
        reason_code_ = ntohs(reason_code_);
        payload_offset += sizeof(reason_code_) + sizeof(response_code_);
    }

    if (control_packet_type_ == NCSI_OEM_COMMAND)
    {
        std::memcpy(&manufacturer_id_, ncsi_buf.data + payload_offset,
                    sizeof(manufacturer_id_));
        manufacturer_id_ = ntohl(manufacturer_id_);
        // Number of reserved bytes after manufacturer_id_ = 3
        oem_command_ =
            *(ncsi_buf.data + payload_offset + sizeof(manufacturer_id_) + 3);
        payload_offset += sizeof(ncsi_oem_extension_header_t);
    }

    packet_raw_ =
        std::vector<uint8_t>(ncsi_buf.data, ncsi_buf.data + ncsi_buf.len);
    // TODO: Verify payload length.

    return true;
}

uint32_t NIC::handle_request(const ncsi_buf_t& request_buf,
                             ncsi_buf_t* response_buf)
{
    const ncsi_header_t* ncsi_header =
        reinterpret_cast<const ncsi_header_t*>(request_buf.data);

    NCSIFrame request_frame;
    request_frame.parse_ethernet_frame(request_buf);
    save_frame_to_log(request_frame);

    uint32_t response_size;
    if (is_loopback_)
    {
        std::memcpy(response_buf, &request_buf, sizeof(request_buf));
        response_size = request_buf.len;
    }
    else if (std::find(simple_commands_.begin(), simple_commands_.end(),
                       ncsi_header->control_packet_type) !=
             simple_commands_.end())
    {
        // Simple Response
        response_size =
            ncsi_build_simple_ack(request_buf.data, response_buf->data);
    }
    else
    {
        // Not-so-Simple Response
        switch (ncsi_header->control_packet_type)
        {
            case NCSI_GET_VERSION_ID:
                response_size = ncsi_build_version_id_ack(
                    request_buf.data, response_buf->data, &version_);
                break;
            case NCSI_GET_CAPABILITIES:
                response_size = sizeof(ncsi_capabilities_response_t);
                {
                    ncsi_capabilities_response_t response;
                    ncsi_build_response_header(
                        request_buf.data, reinterpret_cast<uint8_t*>(&response),
                        0, 0, response_size - sizeof(ncsi_header_t));
                    response.channel_count = channel_count_;
                    std::memcpy(response_buf->data, &response,
                                sizeof(response));
                }
                break;
            case NCSI_GET_PASSTHROUGH_STATISTICS:
                if (is_legacy_)
                {
                    response_size = ncsi_build_pt_stats_legacy_ack(
                        request_buf.data, response_buf->data, &stats_legacy_);
                }
                else
                {
                    response_size = ncsi_build_pt_stats_ack(
                        request_buf.data, response_buf->data, &stats_);
                }
                break;
            case NCSI_GET_LINK_STATUS:
                response_size = ncsi_build_link_status_ack(
                    request_buf.data, response_buf->data, &link_status_);
                break;
            case NCSI_OEM_COMMAND:
                response_size = handle_oem_request(request_buf, response_buf);
                break;
            default:
                response_size = ncsi_build_simple_nack(
                    request_buf.data, response_buf->data, 1, 1);
                break;
        }
    }

    response_buf->len = response_size;

    return response_size;
}

uint32_t NIC::handle_oem_request(const ncsi_buf_t& request_buf,
                                 ncsi_buf_t* response_buf)
{
    const ncsi_oem_simple_cmd_t* oem_cmd =
        reinterpret_cast<const ncsi_oem_simple_cmd_t*>(request_buf.data);
    uint32_t response_size;
    switch (oem_cmd->oem_header.oem_cmd)
    {
        case NCSI_OEM_COMMAND_GET_HOST_MAC:
            response_size = ncsi_build_oem_get_mac_ack(
                request_buf.data, response_buf->data, &mac_);
            break;
        case NCSI_OEM_COMMAND_SET_FILTER:
        {
            const ncsi_oem_set_filter_cmd_t* cmd =
                reinterpret_cast<const ncsi_oem_set_filter_cmd_t*>(
                    request_buf.data);
            if (set_filter(cmd->hdr.channel_id, cmd->filter))
            {
                response_size = ncsi_build_oem_simple_ack(request_buf.data,
                                                          response_buf->data);
            }
            else
            {
                response_size = ncsi_build_simple_nack(
                    request_buf.data, response_buf->data, 3, 4);
            }
        }
        break;
        case NCSI_OEM_COMMAND_ECHO:
            response_size =
                ncsi_build_oem_echo_ack(request_buf.data, response_buf->data);
            break;
        case NCSI_OEM_COMMAND_GET_FILTER:
        {
            const ncsi_simple_command_t* cmd =
                reinterpret_cast<const ncsi_simple_command_t*>(
                    request_buf.data);
            if (cmd->hdr.channel_id == 0)
            {
                response_size = ncsi_build_oem_get_filter_ack(
                    request_buf.data, response_buf->data, &ch0_filter_);
            }
            else if (cmd->hdr.channel_id == 1)
            {
                response_size = ncsi_build_oem_get_filter_ack(
                    request_buf.data, response_buf->data, &ch1_filter_);
            }
            else
            {
                response_size = ncsi_build_simple_nack(
                    request_buf.data, response_buf->data, 3, 4);
            }
        }
        break;
        default:
            response_size = ncsi_build_simple_nack(request_buf.data,
                                                   response_buf->data, 1, 2);
            break;
    }

    return response_size;
}

bool NIC::is_filter_configured(uint8_t channel) const
{
    if (channel == 0)
    {
        return is_ch0_filter_configured_;
    }
    else if (channel == 1)
    {
        return is_ch1_filter_configured_;
    }

    throw std::invalid_argument("Unsupported channel");
}

bool NIC::set_filter(uint8_t channel, const ncsi_oem_filter_t& filter)
{
    ncsi_oem_filter_t* nic_filter;
    if (channel == 0)
    {
        nic_filter = &ch0_filter_;
        is_ch0_filter_configured_ = true;
    }
    else if (channel == 1)
    {
        nic_filter = &ch1_filter_;
        is_ch1_filter_configured_ = true;
    }
    else
    {
        throw std::invalid_argument("Unsupported channel");
    }

    std::memcpy(nic_filter->mac, filter.mac, MAC_ADDR_SIZE);
    nic_filter->ip = 0;
    nic_filter->port = filter.port;
    return true;
}

const ncsi_oem_filter_t& NIC::get_filter(uint8_t channel) const
{
    if (channel == 0)
    {
        return ch0_filter_;
    }
    else if (channel == 1)
    {
        return ch1_filter_;
    }

    throw std::invalid_argument("Unsupported channel");
}

void NIC::set_hostless(bool is_hostless)
{
    auto set_flag_op = [](uint8_t lhs, uint8_t rhs) -> auto
    {
        return lhs | rhs;
    };

    auto clear_flag_op = [](uint8_t lhs, uint8_t rhs) -> auto
    {
        return lhs & ~rhs;
    };

    auto flag_op = is_hostless ? set_flag_op : clear_flag_op;

    if (channel_count_ > 0)
    {
        ch0_filter_.flags =
            flag_op(ch0_filter_.flags, NCSI_OEM_FILTER_FLAGS_HOSTLESS);
    }

    if (channel_count_ > 1)
    {
        ch1_filter_.flags =
            flag_op(ch1_filter_.flags, NCSI_OEM_FILTER_FLAGS_HOSTLESS);
    }
}

void NIC::toggle_hostless()
{
    if (channel_count_ > 0)
    {
        ch0_filter_.flags ^= NCSI_OEM_FILTER_FLAGS_HOSTLESS;
    }

    if (channel_count_ > 1)
    {
        ch1_filter_.flags ^= NCSI_OEM_FILTER_FLAGS_HOSTLESS;
    }
}

bool NIC::is_hostless()
{
    return ch0_filter_.flags & NCSI_OEM_FILTER_FLAGS_HOSTLESS;
}

void NIC::save_frame_to_log(const NCSIFrame& frame)
{
    if (cmd_log_.size() >= max_log_size_)
    {
        cmd_log_.erase(cmd_log_.begin());
    }

    cmd_log_.push_back(frame);
}

const std::vector<uint8_t> NIC::simple_commands_ = {
    NCSI_CLEAR_INITIAL_STATE,
    NCSI_SELECT_PACKAGE,
    NCSI_DESELECT_PACKAGE,
    NCSI_ENABLE_CHANNEL,
    NCSI_DISABLE_CHANNEL,
    NCSI_RESET_CHANNEL,
    NCSI_ENABLE_CHANNEL_NETWORK_TX,
    NCSI_DISABLE_CHANNEL_NETWORK_TX,
    NCSI_AEN_ENABLE,
    NCSI_SET_LINK,
    NCSI_SET_VLAN_FILTER,
    NCSI_ENABLE_VLAN,
    NCSI_DISABLE_VLAN,
    NCSI_SET_MAC_ADDRESS,
    NCSI_ENABLE_BROADCAST_FILTER,
    NCSI_DISABLE_BROADCAST_FILTER,
    NCSI_ENABLE_GLOBAL_MULTICAST_FILTER,
    NCSI_DISABLE_GLOBAL_MULTICAST_FILTER,
    NCSI_SET_NCSI_FLOW_CONTROL,
};

} // namespace mock
