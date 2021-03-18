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

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/ncsi_server.h"


void ncsi_build_response_header(const uint8_t* request_buf,
                                uint8_t* response_buf, uint16_t response_code,
                                uint16_t reason_code, uint16_t payload_length) {
  /* Copy the header from the command */
  memcpy(response_buf, request_buf, sizeof(ncsi_header_t));
  ncsi_simple_response_t* response = (ncsi_simple_response_t*)response_buf;
  response->response_code = response_code;
  response->reason_code = reason_code;

  const ncsi_header_t* request_header = (const ncsi_header_t*)request_buf;
  response->hdr.control_packet_type =
      request_header->control_packet_type | NCSI_RESPONSE;
  response->hdr.payload_length = htons(payload_length);
}

uint32_t ncsi_build_simple_ack(const uint8_t* request_buf,
                               uint8_t* response_buf) {
  /* Copy the header from the command */
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      sizeof(ncsi_simple_response_t) - sizeof(ncsi_header_t));

  return sizeof(ncsi_simple_response_t);
}

uint32_t ncsi_build_simple_nack(const uint8_t* request_buf,
                                uint8_t* response_buf, uint16_t response_code,
                                uint16_t reason_code) {
  ncsi_build_response_header(
      request_buf, response_buf, response_code, reason_code,
      sizeof(ncsi_simple_response_t) - sizeof(ncsi_header_t));

  return sizeof(ncsi_simple_response_t);
}

static void ncsi_build_oem_ack(const uint8_t* request_buf,
                               uint8_t* response_buf, uint32_t response_size) {
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      response_size - sizeof(ncsi_header_t));
  const ncsi_oem_simple_cmd_t* oem_command =
      (const ncsi_oem_simple_cmd_t*)request_buf;
  ncsi_oem_simple_response_t* oem_response =
      (ncsi_oem_simple_response_t*)response_buf;
  memmove(&oem_response->oem_header, &oem_command->oem_header,
          sizeof(ncsi_oem_extension_header_t));
  oem_response->oem_header.manufacturer_id = htonl(NCSI_OEM_MANUFACTURER_ID);
}

uint32_t ncsi_build_version_id_ack(const uint8_t* request_buf,
                                   uint8_t* response_buf,
                                   const ncsi_version_id_t* version_id) {
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      sizeof(ncsi_version_id_response_t) - sizeof(ncsi_header_t));
  ncsi_version_id_response_t* version_id_response =
      (ncsi_version_id_response_t*)response_buf;
  memcpy(&version_id_response->version, version_id, sizeof(ncsi_version_id_t));
  return sizeof(ncsi_version_id_response_t);
}

uint32_t ncsi_build_oem_get_mac_ack(const uint8_t* request_buf,
                                    uint8_t* response_buf,
                                    const mac_addr_t* mac) {
  ncsi_build_oem_ack(request_buf, response_buf,
                     sizeof(ncsi_host_mac_response_t));
  ncsi_host_mac_response_t* response = (ncsi_host_mac_response_t*)response_buf;
  memcpy(response->mac, mac->octet, MAC_ADDR_SIZE);
  return sizeof(ncsi_host_mac_response_t);
}

uint32_t ncsi_build_oem_simple_ack(const uint8_t* request_buf,
                                   uint8_t* response_buf) {
  ncsi_build_oem_ack(request_buf, response_buf,
                     sizeof(ncsi_oem_simple_response_t));
  return sizeof(ncsi_oem_simple_response_t);
}

uint32_t ncsi_build_oem_echo_ack(const uint8_t* request_buf,
                                 uint8_t* response_buf) {
  ncsi_oem_echo_response_t* echo_response =
      (ncsi_oem_echo_response_t*)response_buf;
  const ncsi_oem_echo_cmd_t* echo_cmd = (const ncsi_oem_echo_cmd_t*)request_buf;
  memmove(echo_response->pattern, echo_cmd->pattern, sizeof(echo_cmd->pattern));
  // Because we allow request and response to be the same buffer, it is
  // important that pattern copy precedes the call to ncsi_build_oem_ack.
  ncsi_build_oem_ack(request_buf, response_buf,
                     sizeof(ncsi_oem_echo_response_t));

  return sizeof(ncsi_oem_echo_response_t);
}

uint32_t ncsi_build_oem_get_filter_ack(const uint8_t* request_buf,
                                       uint8_t* response_buf,
                                       const ncsi_oem_filter_t* filter) {
  ncsi_build_oem_ack(request_buf, response_buf,
                     sizeof(ncsi_oem_get_filter_response_t));
  ncsi_oem_get_filter_response_t* get_filter_response =
      (ncsi_oem_get_filter_response_t*)response_buf;
  memcpy(&get_filter_response->filter, filter,
         sizeof(get_filter_response->filter));
  return sizeof(ncsi_oem_get_filter_response_t);
}

uint32_t ncsi_build_pt_stats_ack(const uint8_t* request_buf,
                                 uint8_t* response_buf,
                                 const ncsi_passthrough_stats_t* stats) {
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      sizeof(ncsi_passthrough_stats_response_t) - sizeof(ncsi_header_t));
  ncsi_passthrough_stats_response_t* pt_stats_response =
      (ncsi_passthrough_stats_response_t*)response_buf;
  /* TODO: endianness? */
  memcpy(&pt_stats_response->stats, stats, sizeof(pt_stats_response->stats));
  return sizeof(ncsi_passthrough_stats_response_t);
}

uint32_t ncsi_build_pt_stats_legacy_ack(
    const uint8_t* request_buf, uint8_t* response_buf,
    const ncsi_passthrough_stats_legacy_t* stats) {
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      sizeof(ncsi_passthrough_stats_legacy_response_t) - sizeof(ncsi_header_t));
  ncsi_passthrough_stats_legacy_response_t* pt_stats_response =
      (ncsi_passthrough_stats_legacy_response_t*)response_buf;
  /* TODO: endianness? */
  memcpy(&pt_stats_response->stats, stats, sizeof(pt_stats_response->stats));
  return sizeof(ncsi_passthrough_stats_legacy_response_t);
}

uint32_t ncsi_build_link_status_ack(const uint8_t* request_buf,
                                    uint8_t* response_buf,
                                    const ncsi_link_status_t* link_status) {
  ncsi_build_response_header(
      request_buf, response_buf, 0, 0,
      sizeof(ncsi_link_status_response_t) - sizeof(ncsi_header_t));
  ncsi_link_status_response_t* link_status_response =
      (ncsi_link_status_response_t*)response_buf;
  memcpy(&link_status_response->link_status, link_status,
         sizeof(link_status_response->link_status));
  return sizeof(ncsi_link_status_response_t);
}
