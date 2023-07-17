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

/*
 * Library of NC-SI commands compliant with version 1.0.0.
 *
 * This implements a subset of the commands provided in the specification.
 *
 * Checksums are optional and not implemented here. All NC-SI checksums are set
 * to 0 to indicate that per 8.2.2.3.
 */

#include <stdint.h>
#include <string.h>

#include <netinet/in.h>

#include "platforms/nemora/portable/ncsi_client.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// todo - To save space these tables use the notion that no response is
//            larger than 255 bytes. Need a BUILD_ASSERT.
// todo - Replace 0's with actual sizes once the commands are supported
static const uint8_t ncsi_response_size_table[] = {
  sizeof(ncsi_simple_response_t),        // NCSI_CLEAR_INITIAL_STATE
  sizeof(ncsi_simple_response_t),        // NCSI_SELECT_PACKAGE
  sizeof(ncsi_simple_response_t),        // NCSI_DESELECT_PACKAGE
  sizeof(ncsi_simple_response_t),        // NCSI_ENABLE_CHANNEL
  sizeof(ncsi_simple_response_t),        // NCSI_DISABLE_CHANNEL
  sizeof(ncsi_simple_response_t),        // NCSI_RESET_CHANNEL
  sizeof(ncsi_simple_response_t),        // NCSI_ENABLE_CHANNEL_NETWORK_TX
  sizeof(ncsi_simple_response_t),        // NCSI_DISABLE_CHANNEL_NETWORK_TX
  sizeof(ncsi_simple_response_t),        // NCSI_AEN_ENABLE
  sizeof(ncsi_simple_response_t),        // NCSI_SET_LINK
  sizeof(ncsi_link_status_response_t),   // NCSI_GET_LINK_STATUS
  sizeof(ncsi_simple_response_t),        // NCSI_SET_VLAN_FILTER
  sizeof(ncsi_simple_response_t),        // NCSI_ENABLE_VLAN
  sizeof(ncsi_simple_response_t),        // NCSI_DISABLE_VLAN
  sizeof(ncsi_simple_response_t),        // NCSI_SET_MAC_ADDRESS
  0,                                     // 0x0F is not a valid command
  sizeof(ncsi_simple_response_t),        // NCSI_ENABLE_BROADCAST_FILTER
  sizeof(ncsi_simple_response_t),        // NCSI_DISABLE_BROADCAST_FILTER
  sizeof(ncsi_simple_response_t),        // NCSI_ENABLE_GLOBAL_MULTICAST_FILTER
  sizeof(ncsi_simple_response_t),        // NCSI_DISABLE_GLOBAL_MULTICAST_FILTER
  sizeof(ncsi_simple_response_t),        // NCSI_SET_NCSI_FLOW_CONTROL
  sizeof(ncsi_version_id_response_t),    // NCSI_GET_VERSION_ID
  sizeof(ncsi_capabilities_response_t),  // NCSI_GET_CAPABILITIES
  sizeof(ncsi_parameters_response_t),    // NCSI_GET_PARAMETERS
  0,  // NCSI_GET_CONTROLLER_PACKET_STATISTICS
  0,  // NCSI_GET_NCSI_STATISTICS
  sizeof(ncsi_passthrough_stats_response_t),  // NCSI_GET_PASSTHROUGH_STATISTICS
};

static const uint8_t ncsi_oem_response_size_table[] = {
  sizeof(ncsi_host_mac_response_t),        // NCSI_OEM_COMMAND_GET_HOST_MAC
  sizeof(ncsi_oem_simple_response_t),      // NCSI_OEM_COMMAND_SET_FILTER
  sizeof(ncsi_oem_get_filter_response_t),  // NCSI_OEM_COMMAND_GET_FILTER
  sizeof(ncsi_oem_echo_response_t),        // NCSI_OEM_COMMAND_ECHO
};

// TODO Should increment when we send a packet that is not a retry.
static uint8_t current_instance_id;

/*
 * Sets _most_ of the NC-SI header fields. Caller is expected to set
 * payload_length field if it is > 0. For many NC-SI commands it is 0.
 */
static void set_header_fields(ncsi_header_t* header, uint8_t ch_id,
                              uint8_t cmd_type)
{
  // Destination MAC must be all 0xFF.
  memset(header->ethhdr.dest.octet, 0xFF, sizeof(header->ethhdr.dest.octet));
  // Source MAC can be any value.
  memset(header->ethhdr.src.octet, 0xAB, sizeof(header->ethhdr.src.octet));
  header->ethhdr.ethertype = htons(NCSI_ETHERTYPE);

  // NC-SI Header
  header->mc_id = NCSI_MC_ID;
  header->header_revision = NCSI_HEADER_REV;
  header->reserved_00 = 0;
  header->instance_id = current_instance_id;
  header->control_packet_type = cmd_type;
  header->channel_id = ch_id;
  header->payload_length = 0;  // Caller is expected to set this if != 0.
  memset(header->reserved_01, 0, sizeof(header->reserved_01));
}

uint32_t ncsi_get_response_size(uint8_t cmd_type)
{
  return (cmd_type < ARRAY_SIZE(ncsi_response_size_table))
             ? ncsi_response_size_table[cmd_type]
             : 0;
}

uint32_t ncsi_oem_get_response_size(uint8_t oem_cmd_type)
{
  return (oem_cmd_type < ARRAY_SIZE(ncsi_oem_response_size_table))
             ? ncsi_oem_response_size_table[oem_cmd_type]
             : 0;
}

/*
 * Clear initial state.
 */
uint32_t ncsi_cmd_clear_initial_state(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_CLEAR_INITIAL_STATE);
  return sizeof(ncsi_simple_command_t);
}

/*
 * Set MAC address filters.
 */
uint32_t ncsi_cmd_set_mac(uint8_t* buf, uint8_t channel_id, mac_addr_t* mac)
{
  ncsi_set_mac_command_t* cmd = (ncsi_set_mac_command_t*)buf;

  set_header_fields((ncsi_header_t*)buf, channel_id, NCSI_SET_MAC_ADDRESS);
  cmd->hdr.payload_length =
      htons(sizeof(ncsi_set_mac_command_t) - sizeof(ncsi_header_t));
  memcpy(cmd->mac_addr.octet, mac->octet, sizeof(cmd->mac_addr.octet));
  cmd->mac_addr_num = 1;
  // Unicast MAC address (AT=0), enabled (E=1).
  cmd->misc = 0x01;
  return sizeof(ncsi_set_mac_command_t);
}

uint32_t ncsi_cmd_enable_broadcast_filter(uint8_t* buf, uint8_t channel,
                                          uint32_t filter_settings)
{
  ncsi_enable_broadcast_filter_command_t* cmd =
      (ncsi_enable_broadcast_filter_command_t*)buf;
  set_header_fields((ncsi_header_t*)buf, channel,
                    NCSI_ENABLE_BROADCAST_FILTER);
  cmd->hdr.payload_length = htons(
      sizeof(ncsi_enable_broadcast_filter_command_t) - sizeof(ncsi_header_t));
  cmd->filter_settings = htonl(filter_settings);
  return sizeof(ncsi_enable_broadcast_filter_command_t);
}


uint32_t ncsi_cmd_disable_broadcast_filter(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel,
                    NCSI_DISABLE_BROADCAST_FILTER);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_enable_channel(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_ENABLE_CHANNEL);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_get_link_status(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_GET_LINK_STATUS);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_reset_channel(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_RESET_CHANNEL);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_enable_tx(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel,
                    NCSI_ENABLE_CHANNEL_NETWORK_TX);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_get_version(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_GET_VERSION_ID);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_get_capabilities(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_GET_CAPABILITIES);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_get_parameters(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_GET_PARAMETERS);
  return sizeof(ncsi_simple_command_t);
}

uint32_t ncsi_cmd_get_passthrough_stats(uint8_t* buf, uint8_t channel)
{
  set_header_fields((ncsi_header_t*)buf, channel,
                    NCSI_GET_PASSTHROUGH_STATISTICS);
  return sizeof(ncsi_simple_command_t);
}

/* OEM commands */

uint32_t ncsi_oem_cmd_get_host_mac(uint8_t* buf, uint8_t channel)
{
  ncsi_oem_simple_cmd_t* cmd = (ncsi_oem_simple_cmd_t*)buf;
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_OEM_COMMAND);
  cmd->hdr.payload_length =
      htons(sizeof(ncsi_oem_simple_cmd_t) - sizeof(ncsi_header_t));
  cmd->oem_header.manufacturer_id = htonl(NCSI_OEM_MANUFACTURER_ID);
  memset(cmd->oem_header.reserved, 0, sizeof(cmd->oem_header.reserved));
  cmd->oem_header.oem_cmd = NCSI_OEM_COMMAND_GET_HOST_MAC;
  return sizeof(ncsi_oem_simple_cmd_t);
}

uint32_t ncsi_oem_cmd_get_filter(uint8_t* buf, uint8_t channel)
{
  ncsi_oem_simple_cmd_t* cmd = (ncsi_oem_simple_cmd_t*)buf;
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_OEM_COMMAND);
  cmd->hdr.payload_length =
      htons(sizeof(ncsi_oem_simple_cmd_t) - sizeof(ncsi_header_t));
  cmd->oem_header.manufacturer_id = htonl(NCSI_OEM_MANUFACTURER_ID);
  memset(cmd->oem_header.reserved, 0, sizeof(cmd->oem_header.reserved));
  cmd->oem_header.oem_cmd = NCSI_OEM_COMMAND_GET_FILTER;
  return sizeof(ncsi_oem_simple_cmd_t);
}

uint32_t ncsi_oem_cmd_set_filter(uint8_t* buf, uint8_t channel, mac_addr_t* mac,
                                 uint32_t ip, uint16_t port, uint8_t flags)
{
  ncsi_oem_set_filter_cmd_t* cmd = (ncsi_oem_set_filter_cmd_t*)buf;
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_OEM_COMMAND);
  cmd->hdr.payload_length =
      htons(sizeof(ncsi_oem_set_filter_cmd_t) - sizeof(ncsi_header_t));
  cmd->oem_header.manufacturer_id = htonl(NCSI_OEM_MANUFACTURER_ID);
  memset(cmd->oem_header.reserved, 0, sizeof(cmd->oem_header.reserved));
  cmd->oem_header.oem_cmd = NCSI_OEM_COMMAND_SET_FILTER;

  cmd->filter.reserved0 = 0;
  memcpy(cmd->filter.mac, mac->octet, sizeof(cmd->filter.mac));
  cmd->filter.ip = htonl(ip);
  cmd->filter.port = htons(port);
  cmd->filter.reserved1 = 0;
  cmd->filter.flags = flags;
  memset(cmd->filter.regid, 0, sizeof(cmd->filter.regid));  // reserved for set
  return sizeof(ncsi_oem_set_filter_cmd_t);
}

uint32_t ncsi_oem_cmd_echo(uint8_t* buf, uint8_t channel, uint8_t pattern[64])
{
  ncsi_oem_echo_cmd_t* cmd = (ncsi_oem_echo_cmd_t*)buf;
  set_header_fields((ncsi_header_t*)buf, channel, NCSI_OEM_COMMAND);
  cmd->hdr.payload_length =
      htons(sizeof(ncsi_oem_echo_cmd_t) - sizeof(ncsi_header_t));
  cmd->oem_header.manufacturer_id = htonl(NCSI_OEM_MANUFACTURER_ID);
  memset(cmd->oem_header.reserved, 0, sizeof(cmd->oem_header.reserved));
  cmd->oem_header.oem_cmd = NCSI_OEM_COMMAND_ECHO;
  memcpy(cmd->pattern, pattern, sizeof(cmd->pattern));
  return sizeof(ncsi_oem_echo_cmd_t);
}

ncsi_response_type_t ncsi_validate_response(uint8_t* buf, uint32_t len,
                                            uint8_t cmd_type, bool is_oem,
                                            uint32_t expected_size) {
  if (len < sizeof(ncsi_simple_response_t)) {
    return NCSI_RESPONSE_UNDERSIZED;
  }

  const ncsi_simple_response_t* response = (ncsi_simple_response_t*)buf;
  if (response->response_code || response->reason_code) {
    return NCSI_RESPONSE_NACK;
  }

  const uint8_t std_cmd_type = is_oem ? NCSI_OEM_COMMAND : cmd_type;
  if (response->hdr.control_packet_type != (std_cmd_type | NCSI_RESPONSE)) {
    return NCSI_RESPONSE_UNEXPECTED_TYPE;
  }

  if (len < expected_size ||
      ntohs(response->hdr.payload_length) !=
          expected_size - sizeof(ncsi_header_t)) {
    return NCSI_RESPONSE_UNEXPECTED_SIZE;
  }

  if (is_oem) {
    /* Since the expected_size was checked above, we know that if this is an oem
     * command, it is the right size. */
    const ncsi_oem_simple_response_t* oem_response =
        (ncsi_oem_simple_response_t*)buf;
    if (oem_response->oem_header.manufacturer_id !=
            htonl(NCSI_OEM_MANUFACTURER_ID) ||
        oem_response->oem_header.oem_cmd != cmd_type) {
      return NCSI_RESPONSE_OEM_FORMAT_ERROR;
    }
  }

  return NCSI_RESPONSE_ACK;
}

ncsi_response_type_t ncsi_validate_std_response(uint8_t* buf, uint32_t len,
                                                uint8_t cmd_type) {
  const uint32_t expected_size = ncsi_get_response_size(cmd_type);
  return ncsi_validate_response(buf, len, cmd_type, false, expected_size);
}

ncsi_response_type_t ncsi_validate_oem_response(uint8_t* buf, uint32_t len,
                                                uint8_t cmd_type) {
  const uint32_t expected_size = ncsi_oem_get_response_size(cmd_type);
  return ncsi_validate_response(buf, len, cmd_type, true, expected_size);
}
