#ifndef PLATFORMS_NEMORA_PORTABLE_DEFAULT_ADDRESSES_H_
#define PLATFORMS_NEMORA_PORTABLE_DEFAULT_ADDRESSES_H_
//
// Nemora dedicated port. Filtered by NIC.
#define DEFAULT_ADDRESSES_RX_PORT 3959

// NOTE: All the IPv4 addresses used in this file will be represented in the
//       CPU order and therefore must *not* be used to initialize LWIP
//       ip_addr types, unless the HTONL macro is used.
//
// Example: Given Nemora UDP collector VIP 172.20.0.197, the
//   DEFAULT_ADDRESSES_TARGET_IP macro expands to the 32-bit number 0xAC1408C5
//  (to help the reader: 172 is 0xAC), but with our little endian CPU that
//  32-bit number is represented in memory as:
//  0xC5 @ offset 0, 0x08 @ offset 1, 0x14 @ offset 2, 0xAC @ offset 3
//  Since LWIP uses network order, a correct initialization requires:
//  ip_addr collector = { .addr = HTONL(DEFAULT_ADDRESSES_TARGET_IP) };
//
#ifdef USE_LAB_UDP_DEST
  // Currently targets the lab installer fdcorp1.mtv
  #define DEFAULT_ADDRESSES_TARGET_IP ((172<<24) | (18<<16) | (107<<8) | 1)
  #define DEFAULT_ADDRESSES_TARGET_PORT 50201
#else
  // DEFAULT : Point to production Nemora collector (via anycast VIP).
  #define DEFAULT_ADDRESSES_TARGET_IP ((172<<24) | (20<<16) | (0<<8) | 197)
  #define DEFAULT_ADDRESSES_TARGET_PORT 3960
#endif

// 2001:4860:f802::c5
#define DEFAULT_ADDRESSES_TARGET_IP6 {0x20014860, 0xf8020000, 0, 0xc5}

#ifdef NETWORK_UNITTEST
  #define DEFAULT_ADDRESSES_GATEWAY ((172<<24) | (23<<16) | (130<<8) | 190)
  #define DEFAULT_ADDRESSES_NETMASK ((255<<24) | (255<<16) | (255<<8) | 192)
  #define DEFAULT_ADDRESSES_LOCAL_IP ((172<<24) | (23<<16) | (130<<8) | 141)
  #define DEFAULT_ADDRESSES_MAC {0x00, 0x1a, 0x11, 0x30, 0xc9, 0x6f}
  #define DEFAULT_ADDRESSES_GATEWAY6 {0, 0, 0, 0}
  #define DEFAULT_ADDRESSES_GATEWAY6_MAC {0, 0, 0, 0, 0, 0}
#else
  #define DEFAULT_ADDRESSES_GATEWAY 0
  #define DEFAULT_ADDRESSES_NETMASK 0
  #define DEFAULT_ADDRESSES_LOCAL_IP 0
  #define DEFAULT_ADDRESSES_MAC {0, 0, 0, 0, 0, 0}
  // fe80::1 -- as of 2016-10-13 this is guaranteed to be the GW in prod.
  #define DEFAULT_ADDRESSES_GATEWAY6 {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, \
    0, 0, 0, 0, 0, 1}
  // 02:32:00:00:00:00 -- as of 2016-10-13 this is guaranteed to be the
  // GW MAC addr in prod.
  #define DEFAULT_ADDRESSES_GATEWAY6_MAC {0x02, 0x32, 0, 0, 0, 0}
#endif

#endif  // PLATFORMS_NEMORA_PORTABLE_DEFAULT_ADDRESSES_H_
