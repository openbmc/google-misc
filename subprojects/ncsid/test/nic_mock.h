/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/ncsi_fsm.h"
#include "platforms/nemora/portable/net_types.h"

#include <netinet/in.h>

#include <cstdint>
#include <vector>

namespace mock
{

class NCSIFrame
{
  public:
    mac_addr_t get_dst_mac() const
    {
        return dst_mac_;
    }

    mac_addr_t get_src_mac() const
    {
        return src_mac_;
    }

    uint16_t get_ethertype() const
    {
        return ethertype_;
    }

    bool is_ncsi() const
    {
        return ethertype_ == NCSI_ETHERTYPE;
    }

    uint8_t get_control_packet_type() const
    {
        return control_packet_type_;
    }

    void set_conrol_packet_type(uint8_t control_packet_type)
    {
        control_packet_type_ = control_packet_type;
    }

    bool is_oem_command() const
    {
        return control_packet_type_ == NCSI_OEM_COMMAND;
    }

    uint8_t get_channel_id() const
    {
        return channel_id_;
    }

    void set_channel_id(uint8_t channel_id)
    {
        channel_id_ = channel_id;
    }

    uint8_t get_oem_command() const
    {
        return oem_command_;
    }

    void set_oem_command(uint8_t oem_command)
    {
        set_conrol_packet_type(NCSI_OEM_COMMAND);
        oem_command_ = oem_command;
    }

    uint32_t get_manufacturer_id() const
    {
        return manufacturer_id_;
    }

    std::vector<uint8_t>::size_type get_size() const
    {
        return packet_raw_.size();
    }

    bool is_response() const
    {
        return is_response_;
    }

    uint16_t get_response_code() const
    {
        return response_code_;
    }

    uint16_t get_reason_code() const
    {
        return reason_code_;
    }

    bool parse_ethernet_frame(const ncsi_buf_t& ncsi_buf);

  private:
    mac_addr_t dst_mac_;
    mac_addr_t src_mac_;
    uint16_t ethertype_ = NCSI_ETHERTYPE;
    uint8_t control_packet_type_;
    uint8_t channel_id_;
    uint8_t oem_command_;
    uint32_t manufacturer_id_;
    uint16_t response_code_ = 0;
    uint16_t reason_code_ = 0;
    bool is_response_ = false;
    std::vector<uint8_t> packet_raw_;
};

class NIC
{
  public:
    explicit NIC(bool legacy = false, uint8_t channel_count = 1) :
        channel_count_{channel_count}
    {
        if (legacy)
        {
            version_.firmware_version = htonl(0x08000000);
        }
        else
        {
            version_.firmware_version = 0xabcdef12;
        }

        is_legacy_ = legacy;

        set_link_up();
    }

    void set_link_up()
    {
        link_status_.link_status |= htonl(NCSI_LINK_STATUS_UP);
    }

    void set_mac(const mac_addr_t& mac)
    {
        mac_ = mac;
    }

    mac_addr_t get_mac() const
    {
        return mac_;
    }

    uint8_t get_channel_count() const
    {
        return channel_count_;
    }

    // ????? NICs with Google firmware version ????
    bool is_legacy() const
    {
        return is_legacy_;
    }

    uint32_t handle_request(const ncsi_buf_t& request_buf,
                            ncsi_buf_t* response_buf);

    const std::vector<NCSIFrame>& get_command_log() const
    {
        return cmd_log_;
    }

    bool set_filter(uint8_t channel, const ncsi_oem_filter_t& filter);
    const ncsi_oem_filter_t& get_filter(uint8_t channel) const;

    void set_hostless(bool is_hostless);
    void toggle_hostless();
    bool is_hostless();

    // The NIC itself does not really have a loopback. This is used to emulate
    // the *absence* of NIC and loopback plug inserted.
    void set_loopback()
    {
        is_loopback_ = true;
    }

    void reset_loopback()
    {
        is_loopback_ = false;
    }

    bool is_filter_configured(uint8_t channel) const;

  private:
    static const std::vector<uint8_t> simple_commands_;

    uint32_t handle_oem_request(const ncsi_buf_t& request_buf,
                                ncsi_buf_t* response_buf);

    void save_frame_to_log(const NCSIFrame& frame);

    ncsi_version_id_t version_;
    ncsi_oem_filter_t ch0_filter_;
    ncsi_oem_filter_t ch1_filter_;
    bool is_ch0_filter_configured_ = false;
    bool is_ch1_filter_configured_ = false;
    uint8_t channel_count_;
    mac_addr_t mac_ = {{0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba}};
    std::vector<NCSIFrame> cmd_log_;

    /* If used in a continuous loop, cmd_log_ may grow too big over time.
     * This constant determines how many (most recent) commands will be kept. */
    const uint32_t max_log_size_ = 1000;

    bool is_legacy_;
    bool is_loopback_ = false;

    // TODO: populate stats somehow.
    ncsi_passthrough_stats_t stats_;
    ncsi_passthrough_stats_legacy_t stats_legacy_;

    ncsi_link_status_t link_status_;
};

} // namespace mock
