#ifndef PLATFORMS_NEMORA_PORTABLE_NCSI_H_
#define PLATFORMS_NEMORA_PORTABLE_NCSI_H_

/*
 * Module for interacting with NC-SI capable network cards.
 *
 * DMTF v1.0.0 NC-SI specification:
 * http://www.dmtf.org/sites/default/files/standards/documents/DSP0222_1.0.0.pdf
 */

#include <stdbool.h>
#include <stdint.h>

#include "platforms/nemora/portable/net_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __packed
#define __packed __attribute__((packed))
#endif

// Define states for our NC-SI connection to the NIC.
// There is no mapping to the NC-SI specification for these states, but they
// reflect the outcome of NC-SI commands used in our configuration state
// machine.
//
// 'DOWN' - while in this state, periodically restart the configuration state
//   machine until it succeeds.
// 'LOOPBACK' - the response to the first NC-SI command of the configuration
//   state machine was identical to the command: from this we infer we are in
//   loopback. While in this state, periodically restart the configuration state
//   machine.
// 'UP' - all commands were responded successfully, but need DHCP configuration
//   to go to the next state. While in this state, the connection is tested
//   periodically for failures, which can bring back to 'DOWN'.
// 'UP_AND_CONFIGURED' - NC-SI OEM commands for L3/L4 configuration (which
//   depend on DHCP configuration) were responded successfully. While in this
//   state, the connection and configuration are tested periodically for
//   failures, which can bring back to 'DOWN'.
// 'DISABLED' - reset default state. As soon as network is enabled (which
//   noticeably means that ProdID must be disabled), the state goes to DOWN.
// TODO: connection state has nothing to do with ncsi protocol and needs
// to be moved to ncsi_fsm.h. The main problem with the move is that
// ncsi_client.h defines an extern function with this return type, that is used
// in a lot of places that have no business including ncsi_fsm.h
typedef enum {
  NCSI_CONNECTION_DOWN,
  NCSI_CONNECTION_LOOPBACK,
  NCSI_CONNECTION_UP,
  NCSI_CONNECTION_UP_AND_CONFIGURED,
  NCSI_CONNECTION_DISABLED,
} ncsi_connection_state_t;

typedef enum {
  NCSI_RESPONSE_NONE,
  NCSI_RESPONSE_ACK,
  NCSI_RESPONSE_NACK,
  NCSI_RESPONSE_UNDERSIZED,
  NCSI_RESPONSE_UNEXPECTED_TYPE,
  NCSI_RESPONSE_UNEXPECTED_SIZE,
  NCSI_RESPONSE_OEM_FORMAT_ERROR,
  NCSI_RESPONSE_TIMEOUT,
  NCSI_RESPONSE_UNEXPECTED_PARAMS,
} ncsi_response_type_t;

// For NC-SI Rev 1.0.0, the management controller ID (mc_id) is 0.
#define NCSI_MC_ID 0
// For NC-SI Rev 1.0.0, the header revision is 0x01.
#define NCSI_HEADER_REV 1
#define NCSI_ETHERTYPE 0x88F8
#define NCSI_RESPONSE 0x80

// Command IDs
enum {
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
  NCSI_GET_LINK_STATUS,
  NCSI_SET_VLAN_FILTER,
  NCSI_ENABLE_VLAN,
  NCSI_DISABLE_VLAN,
  NCSI_SET_MAC_ADDRESS,
  // 0x0F is not a valid command
  NCSI_ENABLE_BROADCAST_FILTER = 0x10,
  NCSI_DISABLE_BROADCAST_FILTER,
  NCSI_ENABLE_GLOBAL_MULTICAST_FILTER,
  NCSI_DISABLE_GLOBAL_MULTICAST_FILTER,
  NCSI_SET_NCSI_FLOW_CONTROL,
  NCSI_GET_VERSION_ID,
  NCSI_GET_CAPABILITIES,
  NCSI_GET_PARAMETERS,
  NCSI_GET_CONTROLLER_PACKET_STATISTICS,
  NCSI_GET_NCSI_STATISTICS,
  NCSI_GET_PASSTHROUGH_STATISTICS,
  // 0x1B-0x4F are not valid commands
  NCSI_OEM_COMMAND = 0x50,
};
// OEM Command IDs (subtypes of NCSI_OEM_COMMAND)
#define NCSI_OEM_COMMAND_GET_HOST_MAC 0x00
#define NCSI_OEM_COMMAND_SET_FILTER 0x01
#define NCSI_OEM_COMMAND_GET_FILTER 0x02
#define NCSI_OEM_COMMAND_ECHO 0x03

#define NCSI_OEM_MANUFACTURER_ID 11129  // IANA Enterprise Number for Google.
#define NCSI_OEM_ECHO_PATTERN_SIZE 64

/*
 * NCSI command frame with packet header as described in section 8.2.1.
 * Prepended with an ethernet header.
 */
typedef struct __packed {
  eth_hdr_t ethhdr;
  uint8_t mc_id;
  uint8_t header_revision;
  uint8_t reserved_00;
  uint8_t instance_id;          // Destinguish retried commands from new ones.
  uint8_t control_packet_type;  // See section 8.3, and Table 17.
  uint8_t channel_id;
  uint16_t payload_length;  // In Bytes. Excludes header, checksum, padding.
  uint16_t reserved_01[4];
} ncsi_header_t;

/*
 * Simple NCSI response packet.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
} ncsi_simple_response_t;

/*
 * Simple NCSI command packet.
 */
typedef struct {
  ncsi_header_t hdr;
} ncsi_simple_command_t;

/*
 * Get Link Status Response. 8.4.24
 */
typedef struct __packed {
  uint32_t link_status;
  uint32_t other_indications;
  uint32_t oem_link_status;
} ncsi_link_status_t;

typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_link_status_t link_status;
} ncsi_link_status_response_t;

#define NCSI_LINK_STATUS_UP (1 << 0)

/*
 * Set MAC Address packet. 8.4.31
 */
typedef struct __packed {
  ncsi_header_t hdr;
  mac_addr_t mac_addr;
  uint8_t mac_addr_num;
  uint8_t misc;
} ncsi_set_mac_command_t;

/*
 * Enable Broadcast Filter packet. 8.4.33
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint32_t filter_settings;
} ncsi_enable_broadcast_filter_command_t;

#define NCSI_BROADCAST_FILTER_MASK_ARP         (1 << 0)
#define NCSI_BROADCAST_FILTER_MASK_DHCP_CLIENT (1 << 1)
#define NCSI_BROADCAST_FILTER_MASK_DHCP_SERVER (1 << 2)
#define NCSI_BROADCAST_FILTER_MASK_NETBIOS     (1 << 3)

/*
 * Get Version ID Response. 8.4.44
 */
typedef struct __packed {
  struct {
    uint8_t major;
    uint8_t minor;
    uint8_t update;
    uint8_t alpha1;
    uint8_t reserved[3];
    uint8_t alpha2;
  } ncsi_version;
  uint8_t firmware_name_string[12];
  uint32_t firmware_version;
  uint16_t pci_did;
  uint16_t pci_vid;
  uint16_t pci_ssid;
  uint16_t pci_svid;
  uint32_t manufacturer_id;
} ncsi_version_id_t;

typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_version_id_t version;
} ncsi_version_id_response_t;

/*
 * Get Capabilities Response 8.4.46
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  uint32_t capabilities_flags;
  uint32_t broadcast_packet_filter_capabilties;
  uint32_t multicast_packet_filter_capabilties;
  uint32_t buffering_capability;
  uint32_t aen_control_support;
  uint8_t vlan_filter_count;
  uint8_t mixed_filter_count;
  uint8_t multicast_filter_count;
  uint8_t unicast_filter_count;
  uint16_t reserved;
  uint8_t vlan_mode_support;
  uint8_t channel_count;
} ncsi_capabilities_response_t;

/*
 * Get Parameters Response 8.4.48
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  // TODO: Note: Mellanox 1.4 FW has mac count swapped with mac flags.
  uint8_t mac_address_count;
  uint16_t reserved_01;
  uint8_t mac_address_flags;
  uint8_t vlan_tag_count;
  uint8_t reserved_02;
  uint16_t vlan_tag_flags;
  uint32_t link_settings;
  uint32_t broadcast_settings;
  uint32_t configuration_flags;
  uint8_t vlan_mode;
  uint8_t flow_control_enable;
  uint16_t reserved_03;
  uint32_t aen_control;
  mac_addr_t mac_address[2];
  // TODO: Variable number of mac address filters (max 8. See 8.4.48)
  uint16_t vlan_tags[2];
  // TODO: Variable of vlan filters (up to 15 based on 8.4.48)
} ncsi_parameters_response_t;

/*
 * Get Passthrough statistics response. 8.4.54
 *
 * The legacy data structure matches MLX implementation up to vX
 * (Google vX), however the standard requires the first field to be
 * 64bits and MLX fixed it in vX (Google vX).
 *
 */
typedef struct __packed {
  uint32_t tx_packets_received;  // EC -> NIC
  uint32_t tx_packets_dropped;
  uint32_t tx_channel_errors;
  uint32_t tx_undersized_errors;
  uint32_t tx_oversized_errors;
  uint32_t rx_packets_received;  // Network -> NIC
  uint32_t rx_packets_dropped;
  uint32_t rx_channel_errors;
  uint32_t rx_undersized_errors;
  uint32_t rx_oversized_errors;
} ncsi_passthrough_stats_legacy_t;

typedef struct __packed {
  uint32_t tx_packets_received_hi;  // EC -> NIC (higher 32bit)
  uint32_t tx_packets_received_lo;  // EC -> NIC (lower 32bit)
  uint32_t tx_packets_dropped;
  uint32_t tx_channel_errors;
  uint32_t tx_undersized_errors;
  uint32_t tx_oversized_errors;
  uint32_t rx_packets_received;  // Network -> NIC
  uint32_t rx_packets_dropped;
  uint32_t rx_channel_errors;
  uint32_t rx_undersized_errors;
  uint32_t rx_oversized_errors;
} ncsi_passthrough_stats_t;

typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_passthrough_stats_legacy_t stats;
} ncsi_passthrough_stats_legacy_response_t;

typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_passthrough_stats_t stats;
} ncsi_passthrough_stats_response_t;

/*
 * OEM extension header for custom commands.
 */
typedef struct __packed {
  uint32_t manufacturer_id;
  uint8_t reserved[3];
  uint8_t oem_cmd;
} ncsi_oem_extension_header_t;

/*
 * Response format for simple OEM command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_oem_extension_header_t oem_header;
} ncsi_oem_simple_response_t;

/*
 * Response format for OEM get MAC command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_oem_extension_header_t oem_header;
  uint16_t reserved0;
  uint8_t mac[6];
} ncsi_host_mac_response_t;

/*
 * Format for OEM filter.
 */
typedef struct __packed {
  uint16_t reserved0;
  uint8_t mac[6];
  // If ip is set to zero, the filter will match any IP address, including any
  // IPv6 address.
  uint32_t ip;  // Network order
  uint16_t port;  // Network order
  uint8_t reserved1;
  uint8_t flags;
  uint8_t regid[8];
} ncsi_oem_filter_t;

// Set flags
#define NCSI_OEM_FILTER_FLAGS_ENABLE      (0x01)

// Get flags
#define NCSI_OEM_FILTER_FLAGS_ENABLED     (0x01)
#define NCSI_OEM_FILTER_FLAGS_REGISTERED  (0x02)
#define NCSI_OEM_FILTER_FLAGS_HOSTLESS    (0x04)

/*
 * Command format for simple OEM command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  ncsi_oem_extension_header_t oem_header;
} ncsi_oem_simple_cmd_t;

/*
 * Response format for OEM get filter command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_oem_extension_header_t oem_header;
  ncsi_oem_filter_t filter;
} ncsi_oem_get_filter_response_t;

/*
 * Command format for OEM set filter command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  ncsi_oem_extension_header_t oem_header;
  ncsi_oem_filter_t filter;
} ncsi_oem_set_filter_cmd_t;

/*
 * Command format for OEM echo command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  ncsi_oem_extension_header_t oem_header;
  uint8_t pattern[NCSI_OEM_ECHO_PATTERN_SIZE];
} ncsi_oem_echo_cmd_t;

/*
 * Response format for OEM echo command.
 */
typedef struct __packed {
  ncsi_header_t hdr;
  uint16_t response_code;
  uint16_t reason_code;
  ncsi_oem_extension_header_t oem_header;
  uint8_t pattern[NCSI_OEM_ECHO_PATTERN_SIZE];
} ncsi_oem_echo_response_t;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // PLATFORMS_NEMORA_PORTABLE_NCSI_H_
