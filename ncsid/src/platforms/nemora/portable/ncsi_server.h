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

#ifndef PLATFORMS_NEMORA_PORTABLE_NCSI_SERVER_H_
#define PLATFORMS_NEMORA_PORTABLE_NCSI_SERVER_H_

/*
 * Module for constructing NC-SI response commands on the NIC
 *
 * DMTF v1.0.0 NC-SI specification:
 * http://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.0.0.pdf
 */

#include <stdint.h>

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/net_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Build the header for the response to the command in the buffer.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  response_code: Response Code. Must be zero for ACK.
 *  reason_code: Reason Code. Must be zero for ACK.
 *  payload_length: size of a payload.
 */
void ncsi_build_response_header(const uint8_t* request_buf,
                                uint8_t* response_buf, uint16_t response_code,
                                uint16_t reason_code, uint16_t payload_length);

/*
 * Construct simple ACK command in the buffer, given the buffer with the
 * received command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_simple_ack(const uint8_t* request_buf,
                               uint8_t* response_buf);

/*
 * Construct simple NACK command in the buffer, given the buffer with the
 * received command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  response_code: Response Code
 *  reason_code: Reason Code
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_simple_nack(const uint8_t* request_buf,
                                uint8_t* response_buf, uint16_t response_code,
                                uint16_t reason_code);

/*
 * Construct ACK command in the buffer, given the buffer with the
 * received command, which in this case must be NCSI_GET_VERSION_ID command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  version_id: Version ID struct.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_version_id_ack(const uint8_t* request_buf,
                                   uint8_t* response_buf,
                                   const ncsi_version_id_t* version_id);

/*
 * Construct OEM ACK command in the buffer, given the buffer with the
 * received OEM command, which in this case must be
 * NCSI_OEM_COMMAND_GET_HOST_MAC.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  mac: NIC's MAC address.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_oem_get_mac_ack(const uint8_t* request_buf,
                                    uint8_t* response_buf,
                                    const mac_addr_t* mac);

/*
 * Construct simple OEM ACK command in the buffer, given the buffer with the
 * received command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_oem_simple_ack(const uint8_t* request_buf,
                                   uint8_t* response_buf);

/*
 * Construct OEM ACK command in the buffer, given the buffer with the
 * received command, which in this case must be NCSI_OEM_COMMAND_ECHO.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_oem_echo_ack(const uint8_t* request_buf,
                                 uint8_t* response_buf);

/*
 * Construct ACK response in the buffer, given the buffer with the
 * received NCSI_OEM_COMMAND_GET_FILTER.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  filter: active NIC traffic filter.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_oem_get_filter_ack(const uint8_t* request_buf,
                                       uint8_t* response_buf,
                                       const ncsi_oem_filter_t* filter);

/*
 * Construct ACK response in the buffer, given the buffer with the
 * received NCSI_GET_PASSTHROUGH_STATISTICS command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  stats: ncsi_passthrough_stats_t struct with stats.
 *  TODO): it is not clear what is the endianness of the data in stats
 *  struct. There does not seem to be any conversion on EC side.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_pt_stats_ack(const uint8_t* request_buf,
                                 uint8_t* response_buf,
                                 const ncsi_passthrough_stats_t* stats);

/*
 * This function is similar to ncsi_build_pt_stats_ack, except that it simulates
 * the bug in MLX X - X firmware. It should not be used outside of tests.
 */
uint32_t ncsi_build_pt_stats_legacy_ack(
    const uint8_t* request_buf, uint8_t* response_buf,
    const ncsi_passthrough_stats_legacy_t* stats);

/*
 * Construct ACK response in the buffer, given the buffer with the
 * received NCSI_GET_LINK_STATUS command.
 *
 * Args:
 *  request_buf: buffer containing NC-SI request.
 *  response_buf: buffer, where to put the response. Must be big enough to fit
 *    the response.
 *  link_status: ncsi_link_status_t struct.
 *
 * Returns the size of the response in the buffer.
 */
uint32_t ncsi_build_link_status_ack(const uint8_t* request_buf,
                                    uint8_t* response_buf,
                                    const ncsi_link_status_t* link_status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // PLATFORMS_NEMORA_PORTABLE_NCSI_SERVER_H_
