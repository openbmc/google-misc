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

#ifndef PLATFORMS_NEMORA_PORTABLE_NCSI_CLIENT_H_
#define PLATFORMS_NEMORA_PORTABLE_NCSI_CLIENT_H_

/*
 * Client module for interacting with NC-SI capable network cards.
 *
 * DMTF v1.0.0 NC-SI specification:
 * http://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.0.0.pdf
 */

#include <stdbool.h>
#include <stdint.h>

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/net_types.h"

#define CHANNEL_0_ID 0
#define CHANNEL_1_ID 1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Return the state of our connection to the NIC. Does not map to NC-SI spec.
 * TODO remove this function from here. It is neither defined nor used
 * in ncsi_client.c.
 */
ncsi_connection_state_t ncsi_connection_state(void);

/*
 * Return the expected length for the response to a given NC-SI comamnds
 *
 * Args:
 *  cmd_type: id for the given commands as defined in the NC-SI spec
 *
 * Caveat: returns 0 for commands that have not been implemented yet or for
 *         NCSI_OEM_COMMAND.
 */
uint32_t ncsi_get_response_size(uint8_t cmd_type);

/*
 * Return the expected length for the response to a given OEM NC-SI comamnds
 *
 * Args:
 *  oem_cmd_type: id for the given OEM command as defined in the
 *                ncsi_oem_extension_header_t (and not to be confused with the
 *                id of standard commands)
 */
uint32_t ncsi_oem_get_response_size(uint8_t oem_cmd_type);

/*
 * The following commands write the message to the buffer provided.
 * The length of the message (including the ethernet header and padding) is
 * returned.
 */

/* Standard NC-SI commands */

/*
 * Construct MAC address filtering command. 8.4.15
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_set_mac_command_t) where command will
 *       be placed.
 *  channel: NC-SI channel to filter on (corresponds to a physical port).
 *  mac_filter: MAC address to match against incoming traffic.
 */
uint32_t ncsi_cmd_set_mac(uint8_t* buf, uint8_t channel,
                          mac_addr_t* mac_filter);

/*
 * Construct clear initial state command. 8.4.3
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t) where command will be
 *       placed.
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_clear_initial_state(uint8_t* buf, uint8_t channel);

/*
 * Construct enable broadcast filter command. 8.4.33
 *
 * Args:
 *  filter_settings: filter mask (host order).
 */
uint32_t ncsi_cmd_enable_broadcast_filter(uint8_t* buf, uint8_t channel,
                                          uint32_t filter_settings);
/*
 * Construct disable broadcast filter command. 8.4.35
 *
 * Note: disable filtering == allow forwarding of broadcast traffic
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_disable_broadcast_filter(uint8_t* buf, uint8_t channel);

/*
 * Construct enable channel command. 8.4.9
 *
 * Required before any NC-SI passthrough traffic will go in or out of that
 * channel.
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_enable_channel(uint8_t* buf, uint8_t channel);

/*
 * Construct reset channel command. 8.4.13
 *
 * Put channel into its initial state
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_reset_channel(uint8_t* buf, uint8_t channel);

/*
 * Construct enable TX command. 8.4.15
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_enable_tx(uint8_t* buf, uint8_t channel);

/*
 * Construct get link status command. 8.4.23
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_get_link_status(uint8_t* buf, uint8_t channel);

/*
 * Construct get capabilities command. 8.4.44
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_get_version(uint8_t* buf, uint8_t channel);

/*
 * Construct get capabilities command. 8.4.45
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_get_capabilities(uint8_t* buf, uint8_t channel);

/*
 * Construct get parameters command. 8.4.47
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_get_parameters(uint8_t* buf, uint8_t channel);

/*
 * Construct get pass-through statistics. 8.4.53
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_simple_command_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_cmd_get_passthrough_stats(uint8_t* buf, uint8_t channel);

/* OEM commands */
// TODO: Move OEM commands to another file.

/*
 * Get Host MAC address. Query the NIC for its MAC address(es).
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_oem_simple_cmd_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 */
uint32_t ncsi_oem_cmd_get_host_mac(uint8_t* buf, uint8_t channel);

/*
 * Get filter used for RX traffic.
 */
uint32_t ncsi_oem_cmd_get_filter(uint8_t* buf, uint8_t channel);

/*
 * Set filter for RX traffic. Incoming packets that match all the fields
 * specified here will be forwarded over the NC-SI link.
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_oem_set_filter_cmd_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 *  mac: mac address to filter on (byte array in network order)
 *  ip: IPv4 address to filter on (little-endian)
 *  port: TCP/UDP port number to filter on (little-endian)
 *  flags: bitfield of options.
 */
uint32_t ncsi_oem_cmd_set_filter(uint8_t* buf, uint8_t channel, mac_addr_t* mac,
                                 uint32_t ip, uint16_t port, uint8_t flags);

/*
 * Send NC-SI packet to test connectivity with NIC.
 *
 * Args:
 *  buf: buffer of length >= sizeof(ncsi_oem_echo_cmd_t)
 *  channel: NC-SI channel targeted (corresponds to a physical port).
 *  pattern: echo payload.
 */
uint32_t ncsi_oem_cmd_echo(uint8_t* buf, uint8_t channel,
                           uint8_t pattern[NCSI_OEM_ECHO_PATTERN_SIZE]);

/*
 * Validate NC-SI response in the buffer and return validation result.
 * Exposes "expected_size" as part of interface to handle legacy NICs. Avoid
 * using this function directly, use ncsi_validate_std_response or
 * ncsi_validate_oem_response instead.
 *
 * Args:
 *  buf: buffer containint NC-SI response.
 *  len: size of the response in the buffer.
 *  cmd_type: Id of the command *that was sent* to NIC.
 *  is_eom: true if the response in the buffer is OEM response.
 *  expected_size: expected size of the response.
 */
ncsi_response_type_t ncsi_validate_response(uint8_t* buf, uint32_t len,
                                            uint8_t cmd_type, bool is_oem,
                                            uint32_t expected_size);
/*
 * Validate NC-SI response in the buffer and return validation result.
 *
 * Args:
 *  buf: buffer containint NC-SI response.
 *  len: size of the response in the buffer.
 *  cmd_type: Id of the command *that was sent* to NIC.
 */
ncsi_response_type_t ncsi_validate_std_response(uint8_t* buf, uint32_t len,
                                                uint8_t cmd_type);

/*
 * Validate NC-SI OEM response in the buffer and return validation result.
 *
 * Args:
 *  buf: buffer containint NC-SI response.
 *  len: size of the response in the buffer.
 *  cmd_type: Id of the OEM command *that was sent* to NIC.
 */
ncsi_response_type_t ncsi_validate_oem_response(uint8_t* buf, uint32_t len,
                                                uint8_t cmd_type);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // PLATFORMS_NEMORA_PORTABLE_NCSI_CLIENT_H_
