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

#ifndef PLATFORMS_NEMORA_PORTABLE_NET_TYPES_H_
#define PLATFORMS_NEMORA_PORTABLE_NET_TYPES_H_

#include <stdint.h>

// Buffer big enough for largest frame we expect
// to receive from EMAC (in bytes)
//   1500 (max payload IEEE 802.3) +
//   14 (header) +
//   4 (crc, if not stripped by EMAC) +
//   4 (optional VLAN tag, if not stripped by EMAC)
#define ETH_BUFFER_SIZE 1522
#define IPV4_ETHERTYPE 0x0800
#define IPV6_ADDR_SIZE 16
#define MAC_ADDR_SIZE 6

#ifndef __packed
#define __packed __attribute__((packed))
#endif

/* MAC address */
typedef struct __packed {
  uint8_t octet[MAC_ADDR_SIZE];  // network order
} mac_addr_t;

/*
 * Ethernet header.
 * Note: This assumes a packet without VLAN tags.
 * TODO: configure HW to strip VLAN field.
 */
typedef struct __packed {
  mac_addr_t dest;
  mac_addr_t src;
  uint16_t ethertype;
} eth_hdr_t;

#endif  // PLATFORMS_NEMORA_PORTABLE_NET_TYPES_H_
