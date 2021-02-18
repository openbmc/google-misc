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

#ifndef PLATFORMS_NEMORA_PORTABLE_NCSI_FSM_H_
#define PLATFORMS_NEMORA_PORTABLE_NCSI_FSM_H_

/* Nemora NC-SI (Finite) State Machine implementation */

#include <stdint.h>

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/net_types.h"

/* TODO put this into config somewhere? */
#define NCSI_FSM_RESTART_DELAY_COUNT 100
#define NCSI_FSM_RETEST_DELAY_COUNT 100

/* The network state is defined as a combination of the NC-SI connection state
 * and the network configuration. However the two cannot be decoupled:
 * - we cannot DHCP unless the NC-SI connection is up
 * - we cannot do the OEM L3/L4 NC-SI configuration unless we have a valid
 *   network configuration
 *
 * For additional complexity we cannot get DHCP/ARP responses after the host
 * has loaded the Mellanox NIC driver but we want to be able to periodically
 * test the NC-SI connection regardless of whether we have network configuration
 * (so that flaky cables can be troubleshooted using the host interface).
 *
 * For this reason there are actually 3 NC-SI finite state machines:
 * - L2 configuration (i.e. enabling all available NC-SI channel for passthrough
 *   RX and TX, although only TX will work after the host loads the NIC driver)
 * - L3/L4 configuration (i.e. configuring flow steering for RX traffic that
 *   matches our IP address and dedicated Nemora port so that we can receive
 *   Nemora requests even after the host loaded the NIC driver)
 * - Connection test (i.e. periodically doing a ping test between the EC and the
 *   NIC) and also ensuring that L3/L4 configuration parameters have not been
 *   wiped out)
 *
 * For good karma, try to keep the state machines as linear as possible (one
 * step after the other).
 */

typedef enum {
  // First
  NCSI_STATE_L2_CONFIG_BEGIN,
  // Actual sequence
  NCSI_STATE_RESTART = NCSI_STATE_L2_CONFIG_BEGIN,
  NCSI_STATE_CLEAR_0,
  NCSI_STATE_CLEAR_0_RESPONSE,
  NCSI_STATE_GET_VERSION,
  NCSI_STATE_GET_VERSION_RESPONSE,
  NCSI_STATE_GET_CAPABILITIES,
  NCSI_STATE_GET_CAPABILITIES_RESPONSE,
  NCSI_STATE_CLEAR_1,
  NCSI_STATE_CLEAR_1_RESPONSE,
  NCSI_STATE_RESET_CHANNEL_0,
  NCSI_STATE_RESET_CHANNEL_0_RESPONSE,
  NCSI_STATE_RESET_CHANNEL_1,
  NCSI_STATE_RESET_CHANNEL_1_RESPONSE,
  NCSI_STATE_STOPPED,
  NCSI_STATE_GET_MAC,
  NCSI_STATE_GET_MAC_RESPONSE,
  NCSI_STATE_SET_MAC_FILTER_0,
  NCSI_STATE_SET_MAC_FILTER_0_RESPONSE,
  NCSI_STATE_SET_MAC_FILTER_1,
  NCSI_STATE_SET_MAC_FILTER_1_RESPONSE,
  NCSI_STATE_ENABLE_CHANNEL_0,
  NCSI_STATE_ENABLE_CHANNEL_0_RESPONSE,
  NCSI_STATE_ENABLE_CHANNEL_1,
  NCSI_STATE_ENABLE_CHANNEL_1_RESPONSE,
  NCSI_STATE_ENABLE_TX,
  NCSI_STATE_ENABLE_TX_RESPONSE,
  // Last
  NCSI_STATE_L2_CONFIG_END
} ncsi_l2_config_state_t;

typedef enum {
  // First
  NCSI_STATE_L3L4_CONFIG_BEGIN,
  // Actual sequence
  NCSI_STATE_CONFIG_FILTERS,
  // Last
  NCSI_STATE_L3L4_CONFIG_END
} ncsi_l3l4_config_state_t;

typedef enum {
  // First
  NCSI_STATE_TEST_BEGIN,
  // Actual sequence
  NCSI_STATE_TEST_PARAMS = NCSI_STATE_TEST_BEGIN,
  NCSI_STATE_ECHO,
  NCSI_STATE_ECHO_RESPONSE,
  NCSI_STATE_CHECK_FILTERS,
  NCSI_STATE_CHECK_FILTERS_RESPONSE,
  NCSI_STATE_GET_PT_STATS,
  NCSI_STATE_GET_PT_STATS_RESPONSE,
  NCSI_STATE_GET_LINK_STATUS,
  NCSI_STATE_GET_LINK_STATUS_RESPONSE,
  // Last
  NCSI_STATE_TEST_END
} ncsi_test_state_t;

typedef struct {
  ncsi_l2_config_state_t l2_config_state;
  ncsi_l3l4_config_state_t l3l4_config_state;
  ncsi_test_state_t test_state;
  // Last (OEM) command that was sent. (L3L4 SM only)
  // Valid only if l3l4_waiting_response is true.
  uint8_t l3l4_command;
  // Number of the channel we are currently operating on. (L3L4 SM only)
  uint8_t l3l4_channel;
  // If true, means the request was sent and we are waiting for response.
  bool l3l4_waiting_response;
  uint8_t channel_count;
  // The re-start and re-test delays ensures that we can flush the DMA
  // buffers of potential out-of-sequence NC-SI packets (e.g. from
  // packet that may have been received shortly after we timed out on
  // them). The re-test delays also reduce the effect of NC-SI
  // testing on more useful traffic.
  uint8_t restart_delay_count;
  uint8_t retest_delay_count;
  struct {
    uint8_t flags;
    uint8_t regid[8];
  } flowsteering[2];
} ncsi_state_t;

// Debug variables.
// TODO - Change name to something more meaningful since the NC-SI test
//   is not a debug-only feature.
typedef struct {
  uint32_t task_count;
  uint32_t host_ctrl_flags;
  struct {
    bool enabled;
    bool pending_stop;
    bool pending_restart;
    bool oem_filter_disable;
    bool loopback;
    bool mlx_legacy;
    uint32_t fail_count;
    ncsi_state_t state_that_failed;
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t tx_error_count;
    struct {
      uint32_t timeout_count;
      uint32_t oversized_count;
      uint32_t undersized_count;
      uint32_t nack_count;
      uint32_t unexpected_size_count;
      uint32_t unexpected_type_count;
    } rx_error;
    struct {
      uint32_t runs;
      uint8_t ch_under_test;
      uint8_t tries;
      uint8_t max_tries;  // 0 = skip test, 1 = restart on failure, > 1 = retry
      struct {
        uint8_t tx[NCSI_OEM_ECHO_PATTERN_SIZE];
        uint32_t tx_count;
        uint32_t rx_count;
        uint32_t bad_rx_count;
        uint8_t last_bad_rx[NCSI_OEM_ECHO_PATTERN_SIZE];
      } ping;
    } test;
    ncsi_passthrough_stats_t pt_stats_be[2];  // big-endian as received from NIC
  } ncsi;
} network_debug_t;

typedef struct {
  uint8_t data[ETH_BUFFER_SIZE];
  uint32_t len;  // Non-zero when there's a new NC-SI response.
} ncsi_buf_t;


#ifdef __cplusplus
extern "C" {
#endif

ncsi_response_type_t ncsi_fsm_poll_l2_config(ncsi_state_t* ncsi_state,
                                             network_debug_t* network_debug,
                                             ncsi_buf_t* ncsi_buf,
                                             mac_addr_t* mac);

ncsi_response_type_t ncsi_fsm_poll_l3l4_config(ncsi_state_t* ncsi_state,
                                               network_debug_t* network_debug,
                                               ncsi_buf_t* ncsi_buf,
                                               mac_addr_t* mac,
                                               uint32_t ipv4_addr,
                                               uint16_t rx_port);

ncsi_response_type_t ncsi_fsm_poll_test(ncsi_state_t* ncsi_state,
                                        network_debug_t* network_debug,
                                        ncsi_buf_t* ncsi_buf, mac_addr_t* mac,
                                        uint32_t ipv4_addr, uint16_t rx_port);

/*
 * Report a global state of the NC-SI connection as a function of the state
 * of the 3 finite state machines.
 * Note: Additionally for the case where the connection is down it reports
 *   whether a loopback is inferred.
 */
ncsi_connection_state_t ncsi_fsm_connection_state(
    const ncsi_state_t* ncsi_state, const network_debug_t* network_debug);

/*
 * Returns true if we have executed an NC-SI Get OEM Filter command for all
 * channels and the flags indicate that it is running in hostless mode.
 * This means that we can DHCP/ARP if needed.
 * Otherwise returns false.
 *
 * NOTE: We default to false, if we cannot complete the L2 config state
 *   machine or the test sequence.
 */
bool ncsi_fsm_is_nic_hostless(const ncsi_state_t* ncsi_state);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // PLATFORMS_NEMORA_PORTABLE_NCSI_FSM_H_
