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

#include <string.h>

#include <netinet/in.h>

#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/ncsi_client.h"
#include "platforms/nemora/portable/ncsi_fsm.h"

#define GO_TO_STATE(variable, state) do { *variable = state; } while (0)
#define GO_TO_NEXT_STATE(variable) do { (*variable)++; } while (0)

// TODO - This state machine needs to be rewritten, now that we have a
// better idea of the states and transitions involved.
// The NC-SI related states of the state machine are currently organized in
// request/response pairs. However when I added support for the second channel
// this resulted in more hard-coded pairs which worked okay for X
// (despite some ugliness, see ch_under_test below) but broke down for X
// since it only supports 1 channel. For now just add a little more ugliness
// by stepping by 1 or 3 when going from a pair to the next depending on whether
// the second channel is supported (1) or not (3 - skip over the second channel
// pair).
#define GO_TO_NEXT_CHANNEL(variable, ncsi_state)\
  do { *variable += (ncsi_state->channel_count == 1)\
      ? 3 : 1; } while (0)

static void ncsi_fsm_clear_state(ncsi_state_t* ncsi_state) {
  // This implicitly resets:
  //   l2_config_state   to NCSI_STATE_L2_CONFIG_BEGIN
  //   l3l4_config_state to NCSI_STATE_L3L4_CONFIG_BEGIN
  //   test_state        to NCSI_STATE_TEST_BEGIN
  memset(ncsi_state, 0, sizeof(ncsi_state_t));
}

static void ncsi_fsm_fail(ncsi_state_t* ncsi_state,
                          network_debug_t* network_debug) {
  network_debug->ncsi.fail_count++;
  memcpy(&network_debug->ncsi.state_that_failed, ncsi_state,
         sizeof(network_debug->ncsi.state_that_failed));
  ncsi_fsm_clear_state(ncsi_state);
}

ncsi_connection_state_t ncsi_fsm_connection_state(
    const ncsi_state_t* ncsi_state, const network_debug_t* network_debug) {
  if (!network_debug->ncsi.enabled) {
    return NCSI_CONNECTION_DISABLED;
  }
  if (ncsi_state->l2_config_state != NCSI_STATE_L2_CONFIG_END) {
    if (network_debug->ncsi.loopback) {
      return NCSI_CONNECTION_LOOPBACK;
    } else {
      return NCSI_CONNECTION_DOWN;
    }
  }
  if (ncsi_state->l3l4_config_state != NCSI_STATE_L3L4_CONFIG_END) {
    return NCSI_CONNECTION_UP;
  }
  return NCSI_CONNECTION_UP_AND_CONFIGURED;
}

ncsi_response_type_t ncsi_fsm_poll_l2_config(ncsi_state_t* ncsi_state,
                                             network_debug_t* network_debug,
                                             ncsi_buf_t* ncsi_buf,
                                             mac_addr_t* mac) {
  ncsi_l2_config_state_t* const state_variable = &ncsi_state->l2_config_state;
  ncsi_response_type_t ncsi_response_type = NCSI_RESPONSE_NONE;
  uint32_t len = 0;

  switch(*state_variable) {
  case NCSI_STATE_RESTART:
    if (++ncsi_state->restart_delay_count >= NCSI_FSM_RESTART_DELAY_COUNT) {
      network_debug->ncsi.pending_restart = false;
      GO_TO_NEXT_STATE(state_variable);
      ncsi_state->restart_delay_count = 0;
    }
    break;
  case NCSI_STATE_CLEAR_0: // necessary to get mac
    len = ncsi_cmd_clear_initial_state(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_CLEAR_0_RESPONSE:
    {
      bool loopback = false;
      ncsi_response_type = ncsi_validate_std_response(
          ncsi_buf->data, ncsi_buf->len, NCSI_CLEAR_INITIAL_STATE);
      if (NCSI_RESPONSE_ACK == ncsi_response_type) {
        GO_TO_NEXT_STATE(state_variable);
      } else {
        // If we did not receive a response but we did receive something,
        // then maybe there is a physical loopback, so check that we received
        // exactly what we sent
        if (ncsi_buf->len >= sizeof(ncsi_simple_command_t)) {
          ncsi_simple_command_t expected_loopback_data;
          (void)ncsi_cmd_clear_initial_state((uint8_t*)&expected_loopback_data,
                                             CHANNEL_0_ID);
          if (0 == memcmp((uint8_t*)&expected_loopback_data,
                          ncsi_buf->data, sizeof(expected_loopback_data))) {
            loopback = true;
          }
        }
        ncsi_fsm_fail(ncsi_state, network_debug);
      }
      network_debug->ncsi.loopback = loopback;
    }
    break;
  case NCSI_STATE_GET_VERSION:
    len = ncsi_cmd_get_version(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_GET_VERSION_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_GET_VERSION_ID);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      ncsi_version_id_response_t* get_version_response =
          (ncsi_version_id_response_t*)ncsi_buf->data;
      // TODO - Add check for this being actually X
      network_debug->ncsi.mlx_legacy =
          ((ntohl(get_version_response->version.firmware_version) >> 24) ==
           0x08);
      GO_TO_NEXT_CHANNEL(state_variable, ncsi_state);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_GET_CAPABILITIES:
    len = ncsi_cmd_get_capabilities(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_GET_CAPABILITIES_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_GET_CAPABILITIES);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      const ncsi_capabilities_response_t* get_capabilities_response =
        (ncsi_capabilities_response_t*) ncsi_buf->data;
      if (1 != get_capabilities_response->channel_count &&
          2 != get_capabilities_response->channel_count) {
        /* TODO: Return Error
        CPRINT("[NCSI Unsupported channel count {}]\n",
                get_capabilities_response->channel_count);
          */
        ncsi_fsm_fail(ncsi_state, network_debug);
      } else {
        ncsi_state->channel_count =
          get_capabilities_response->channel_count;
        GO_TO_NEXT_CHANNEL(state_variable, ncsi_state);
      }
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_CLEAR_1:
    len = ncsi_cmd_clear_initial_state(ncsi_buf->data, CHANNEL_1_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_CLEAR_1_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_CLEAR_INITIAL_STATE);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_STATE(state_variable);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_RESET_CHANNEL_0:
    if (network_debug->ncsi.pending_stop) {
      len = ncsi_cmd_reset_channel(ncsi_buf->data, CHANNEL_0_ID);
      GO_TO_NEXT_STATE(state_variable);
    } else {
      // skip resetting channels
      GO_TO_STATE(state_variable, NCSI_STATE_GET_MAC);
    }
    break;
  case NCSI_STATE_RESET_CHANNEL_0_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_RESET_CHANNEL);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_CHANNEL(state_variable, ncsi_state);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_RESET_CHANNEL_1:
    len = ncsi_cmd_reset_channel(ncsi_buf->data, CHANNEL_1_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_RESET_CHANNEL_1_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_RESET_CHANNEL);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_STATE(state_variable);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_STOPPED:
    network_debug->ncsi.pending_stop = false;
    // Reset the L2 config state machine through fail(). This state machine
    // will not be executed again so long as 'enabled' is false.
    network_debug->ncsi.enabled = false;
    ncsi_fsm_fail(ncsi_state, network_debug);
    break;
    // TODO: Add check for MFG ID and firmware version before trying
    // any OEM commands.
  case NCSI_STATE_GET_MAC:
    // Only get MAC from channel 0, because that's the one that identifies the
    // host machine (for both MDB and DHCP).
    len = ncsi_oem_cmd_get_host_mac(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_GET_MAC_RESPONSE:
    ncsi_response_type = ncsi_validate_oem_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_OEM_COMMAND_GET_HOST_MAC);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      ncsi_host_mac_response_t* get_mac_response =
        (ncsi_host_mac_response_t*) ncsi_buf->data;
      memcpy(mac->octet, get_mac_response->mac, sizeof(mac_addr_t));
      GO_TO_NEXT_STATE(state_variable);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_SET_MAC_FILTER_0:
    len = ncsi_cmd_set_mac(ncsi_buf->data, CHANNEL_0_ID, mac);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_SET_MAC_FILTER_0_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_SET_MAC_ADDRESS);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_CHANNEL(state_variable, ncsi_state);
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_SET_MAC_FILTER_1:
    len = ncsi_cmd_set_mac(ncsi_buf->data, CHANNEL_1_ID, mac);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_SET_MAC_FILTER_1_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_SET_MAC_ADDRESS);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_STATE(state_variable);
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_ENABLE_CHANNEL_0:
    len = ncsi_cmd_enable_channel(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_ENABLE_CHANNEL_0_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_ENABLE_CHANNEL);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_CHANNEL(state_variable, ncsi_state);
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_ENABLE_CHANNEL_1:
    len = ncsi_cmd_enable_channel(ncsi_buf->data, CHANNEL_1_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_ENABLE_CHANNEL_1_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_ENABLE_CHANNEL);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_STATE(state_variable);
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  // TODO: Enable broadcast filter to block ARP.
  case NCSI_STATE_ENABLE_TX:
    // The NIC FW transmits all passthrough TX on the lowest enabled channel,
    // so there is no point in enabling TX on the second channel.
    // TODO: - In the future we may add a check for link status,
    //         in which case we may want to intelligently disable ch.0
    //         (if down) and enable ch.1
    len = ncsi_cmd_enable_tx(ncsi_buf->data, CHANNEL_0_ID);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_ENABLE_TX_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_ENABLE_CHANNEL_NETWORK_TX);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      GO_TO_NEXT_STATE(state_variable);
    } else{
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_L2_CONFIG_END:
    // Done
    break;
  default:
    ncsi_fsm_fail(ncsi_state, network_debug);
    break;
  }

  ncsi_buf->len = len;
  return ncsi_response_type;
}

static uint32_t write_ncsi_oem_config_filter(uint8_t* buffer, uint8_t channel,
                                             network_debug_t* network_debug,
                                             mac_addr_t* mac,
                                             uint32_t ipv4_addr,
                                             uint16_t rx_port) {
  uint32_t len;
  (void)ipv4_addr;
  if (network_debug->ncsi.oem_filter_disable) {
    mac_addr_t zero_mac = {.octet = {0,}};
    len = ncsi_oem_cmd_set_filter(buffer, channel, &zero_mac, 0, 0, 0);

  } else {
    len = ncsi_oem_cmd_set_filter(buffer, channel, mac, 0, rx_port, 1);
  }
  return len;
}

ncsi_response_type_t ncsi_fsm_poll_l3l4_config(ncsi_state_t* ncsi_state,
                                               network_debug_t* network_debug,
                                               ncsi_buf_t* ncsi_buf,
                                               mac_addr_t* mac,
                                               uint32_t ipv4_addr,
                                               uint16_t rx_port) {
  uint32_t len = 0;
  ncsi_response_type_t ncsi_response_type = NCSI_RESPONSE_NONE;

  if (ncsi_state->l3l4_config_state == NCSI_STATE_L3L4_CONFIG_BEGIN) {
    ncsi_state->l3l4_channel = 0;
    ncsi_state->l3l4_waiting_response = false;
    ncsi_state->l3l4_config_state = NCSI_STATE_CONFIG_FILTERS;
  }

  /* Go through every state with every channel. */
  if (ncsi_state->l3l4_waiting_response) {
    ncsi_response_type = ncsi_validate_oem_response(
        ncsi_buf->data, ncsi_buf->len, ncsi_state->l3l4_command);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      /* Current channel ACK'ed, go to the next one. */
      ncsi_state->l3l4_channel++;
      if (ncsi_state->l3l4_channel >= ncsi_state->channel_count) {
        /* All channels done, reset channel number and go to the next state.
         * NOTE: This assumes that state numbers are sequential.*/
        ncsi_state->l3l4_config_state += 1;
        ncsi_state->l3l4_channel = 0;
      }
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }

    ncsi_state->l3l4_waiting_response = false;
  } else {
    // Send appropriate command.
    switch(ncsi_state->l3l4_config_state) {
      case NCSI_STATE_CONFIG_FILTERS:
        len =
            write_ncsi_oem_config_filter(ncsi_buf->data, ncsi_state->l3l4_channel,
                                         network_debug, mac, ipv4_addr, rx_port);
        ncsi_state->l3l4_command = NCSI_OEM_COMMAND_SET_FILTER;
        ncsi_state->l3l4_waiting_response = true;
        break;
      default:
        ncsi_fsm_fail(ncsi_state, network_debug);
        break;
    }
  }

  ncsi_buf->len = len;
  return ncsi_response_type;
}

/*
 * Start a sub-section of the state machine that runs health checks.
 * This is dependent on the NC-SI configuration being completed
 * (e.g. ncsi_channel_count must be known).
 */
static bool ncsi_fsm_start_test(network_debug_t* network_debug,
                                uint8_t channel_count) {
  if (network_debug->ncsi.test.max_tries > 0) {
    network_debug->ncsi.test.runs++;
    if (2 == channel_count) {
      network_debug->ncsi.test.ch_under_test ^= 1;
    } else {
      network_debug->ncsi.test.ch_under_test = 0;
    }
    return true;
  }
  return false;
}

/*
 * Allow for a limited number of retries for the NC-SI test because
 * it can fail under heavy TCP/IP load (since NC-SI responses share
 * the RX buffers in chip/$(CHIP)/net.c with TCP/IP incoming traffic).
 */
static bool ncsi_fsm_retry_test(network_debug_t* network_debug) {
  const uint8_t max_tries = network_debug->ncsi.test.max_tries;
  if (max_tries) {
    uint8_t remaining_tries = max_tries - 1 - network_debug->ncsi.test.tries;
    if (remaining_tries > 0) {
      network_debug->ncsi.test.tries++;
      return true;
    }
  }
  network_debug->ncsi.test.tries = 0;
  return false;
}

/*
 * Returns true if we have executed an NC-SI Get OEM Filter command for all
 * channels and the flags indicate that it is running in hostless mode.
 * This means that we can DHCP/ARP if needed.
 * Otherwise returns false.
 *
 * NOTE: We default to false, if we cannot complete the L2 config state
 *   machine or the test sequence.
 */
bool ncsi_fsm_is_nic_hostless(const ncsi_state_t* ncsi_state) {
  uint8_t flags = ncsi_state->flowsteering[0].flags;
  if (ncsi_state->channel_count > 1) {
    flags &= ncsi_state->flowsteering[1].flags;
  }
  return flags & NCSI_OEM_FILTER_FLAGS_HOSTLESS;
}

static void ncsi_fsm_update_passthrough_stats(
    const ncsi_passthrough_stats_t* increment, network_debug_t* network_debug) {
  ncsi_passthrough_stats_t* accumulated =
      &network_debug->ncsi.pt_stats_be[network_debug->ncsi.test.ch_under_test];
#define ACCUMULATE_PT_STATS(stat) accumulated->stat += increment->stat;
  ACCUMULATE_PT_STATS(tx_packets_received_hi);
  ACCUMULATE_PT_STATS(tx_packets_received_lo);
  ACCUMULATE_PT_STATS(tx_packets_dropped);
  ACCUMULATE_PT_STATS(tx_channel_errors);
  ACCUMULATE_PT_STATS(tx_undersized_errors);
  ACCUMULATE_PT_STATS(tx_oversized_errors);
  ACCUMULATE_PT_STATS(rx_packets_received);
  ACCUMULATE_PT_STATS(rx_packets_dropped);
  ACCUMULATE_PT_STATS(rx_channel_errors);
  ACCUMULATE_PT_STATS(rx_undersized_errors);
  ACCUMULATE_PT_STATS(rx_oversized_errors);
#undef ACCUMULATE_PT_STATS
}

static void ncsi_fsm_update_passthrough_stats_legacy(
    const ncsi_passthrough_stats_legacy_t* read,
    network_debug_t* network_debug) {
  // Legacy MLX response does not include tx_packets_received_hi and also MLX
  // counters
  // are not reset on read (i.e. we cannot accumulate them).
  ncsi_passthrough_stats_t* accumulated =
      &network_debug->ncsi.pt_stats_be[network_debug->ncsi.test.ch_under_test];
  accumulated->tx_packets_received_hi = 0;
  accumulated->tx_packets_received_lo = read->tx_packets_received;
#define COPY_PT_STATS(stat) accumulated->stat = read->stat;
  COPY_PT_STATS(tx_packets_dropped);
  COPY_PT_STATS(tx_channel_errors);
  COPY_PT_STATS(tx_undersized_errors);
  COPY_PT_STATS(tx_oversized_errors);
  COPY_PT_STATS(rx_packets_received);
  COPY_PT_STATS(rx_packets_dropped);
  COPY_PT_STATS(rx_channel_errors);
  COPY_PT_STATS(rx_undersized_errors);
  COPY_PT_STATS(rx_oversized_errors);
#undef COPY_PT_STATS
}

ncsi_response_type_t ncsi_fsm_poll_test(ncsi_state_t* ncsi_state,
                                        network_debug_t* network_debug,
                                        ncsi_buf_t* ncsi_buf, mac_addr_t* mac,
                                        uint32_t ipv4_addr, uint16_t rx_port) {
  ncsi_test_state_t* const state_variable =
      &ncsi_state->test_state;
  ncsi_response_type_t ncsi_response_type = NCSI_RESPONSE_NONE;
  uint32_t len = 0;

  switch(*state_variable) {
  case NCSI_STATE_TEST_PARAMS:
    if (ncsi_fsm_start_test(network_debug, ncsi_state->channel_count)) {
      GO_TO_NEXT_STATE(state_variable);
    } else {
      // debugging only - skip test by setting max_tries to 0
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_END);
    }
    break;
  case NCSI_STATE_ECHO:
    len = ncsi_oem_cmd_echo(ncsi_buf->data,
                            network_debug->ncsi.test.ch_under_test,
                            network_debug->ncsi.test.ping.tx);
    network_debug->ncsi.test.ping.tx_count++;
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_ECHO_RESPONSE:
    ncsi_response_type = ncsi_validate_oem_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_OEM_COMMAND_ECHO);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      network_debug->ncsi.test.ping.rx_count++;
      ncsi_oem_echo_response_t* echo_response =
        (ncsi_oem_echo_response_t*) ncsi_buf->data;
      if (0 == memcmp(echo_response->pattern,
                      network_debug->ncsi.test.ping.tx,
                      sizeof(echo_response->pattern))) {
        GO_TO_NEXT_STATE(state_variable);
        break;
      } else {
        network_debug->ncsi.test.ping.bad_rx_count++;
        memcpy(network_debug->ncsi.test.ping.last_bad_rx,
               echo_response->pattern,
               sizeof(network_debug->ncsi.test.ping.last_bad_rx));
      }
    }
    if (ncsi_fsm_retry_test(network_debug)) {
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_BEGIN);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_CHECK_FILTERS:
    len = ncsi_oem_cmd_get_filter(ncsi_buf->data,
                                  network_debug->ncsi.test.ch_under_test);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_CHECK_FILTERS_RESPONSE:
    ncsi_response_type = ncsi_validate_oem_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_OEM_COMMAND_GET_FILTER);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      ncsi_oem_get_filter_response_t* get_filter_response =
        (ncsi_oem_get_filter_response_t*) ncsi_buf->data;
      // Stash away response because it contains information about NIC mode
      memcpy((void*)ncsi_state->flowsteering[
          network_debug->ncsi.test.ch_under_test].regid,
             (void*)get_filter_response->filter.regid,
             sizeof(ncsi_state->flowsteering[0].regid));
      ncsi_state->flowsteering[
          network_debug->ncsi.test.ch_under_test].flags =
          get_filter_response->filter.flags;
      // Test filter parameters only if we know that we configured the NIC,
      // and if the NIC is in host-based mode (it appears to return all zeros's
      // in hostless mode!).
      if (NCSI_STATE_L3L4_CONFIG_END != ncsi_state->l3l4_config_state ||
          ncsi_fsm_is_nic_hostless(ncsi_state)) {
        GO_TO_NEXT_STATE(state_variable);
        break;
      }
      ncsi_oem_set_filter_cmd_t expected;
      (void)write_ncsi_oem_config_filter(
          (uint8_t*)&expected, network_debug->ncsi.test.ch_under_test,
          network_debug, mac, ipv4_addr, rx_port);
      /* TODO: handle these responses in error reporting routine */
      if (0 != memcmp((void*)&get_filter_response->filter.mac,
                       (void*)&expected.filter.mac,
                       sizeof(expected.filter.mac))) {
        ncsi_response_type = NCSI_RESPONSE_UNEXPECTED_PARAMS;
      } else if (get_filter_response->filter.ip != expected.filter.ip ||
            get_filter_response->filter.port != expected.filter.port) {
        ncsi_response_type = NCSI_RESPONSE_UNEXPECTED_PARAMS;
      } else {
        GO_TO_NEXT_STATE(state_variable);
        break;
      }
    }
    if (ncsi_fsm_retry_test(network_debug)) {
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_BEGIN);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_GET_PT_STATS:
    len = ncsi_cmd_get_passthrough_stats(
        ncsi_buf->data, network_debug->ncsi.test.ch_under_test);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_GET_PT_STATS_RESPONSE:
    if (!network_debug->ncsi.mlx_legacy) {
      ncsi_response_type = ncsi_validate_std_response(
          ncsi_buf->data, ncsi_buf->len, NCSI_GET_PASSTHROUGH_STATISTICS);
      if (ncsi_response_type == NCSI_RESPONSE_ACK) {
        const ncsi_passthrough_stats_response_t* response =
            (const ncsi_passthrough_stats_response_t*)ncsi_buf->data;
        ncsi_fsm_update_passthrough_stats(&response->stats, network_debug);
        GO_TO_NEXT_STATE(state_variable);
        break;
      }
    } else {
      uint32_t response_size =
          ncsi_get_response_size(NCSI_GET_PASSTHROUGH_STATISTICS) -
          sizeof(uint32_t);
      ncsi_response_type = ncsi_validate_response(
          ncsi_buf->data, ncsi_buf->len, NCSI_GET_PASSTHROUGH_STATISTICS, false,
          response_size);
      if (NCSI_RESPONSE_ACK == ncsi_response_type) {
        const ncsi_passthrough_stats_legacy_response_t* legacy_response =
            (const ncsi_passthrough_stats_legacy_response_t*)ncsi_buf->data;
        ncsi_fsm_update_passthrough_stats_legacy(&legacy_response->stats,
                                                 network_debug);
        GO_TO_NEXT_STATE(state_variable);
        break;
      }
    }
    if (ncsi_fsm_retry_test(network_debug)) {
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_BEGIN);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_GET_LINK_STATUS:
    // We only care about ch.0 link status because that's the only one we use
    // to transmit.
    len = ncsi_cmd_get_link_status(ncsi_buf->data, 0);
    GO_TO_NEXT_STATE(state_variable);
    break;
  case NCSI_STATE_GET_LINK_STATUS_RESPONSE:
    ncsi_response_type = ncsi_validate_std_response(
        ncsi_buf->data, ncsi_buf->len, NCSI_GET_LINK_STATUS);
    if (NCSI_RESPONSE_ACK == ncsi_response_type) {
      const ncsi_link_status_response_t* response =
          (const ncsi_link_status_response_t*)ncsi_buf->data;
      const uint32_t link_status = ntohl(response->link_status.link_status);
      if (link_status & NCSI_LINK_STATUS_UP) {
        GO_TO_NEXT_STATE(state_variable);
        break;
      }
      // TODO: report this error.
      // CPRINT("[NCSI Link Status down {:#08x}]\n", link_status);
    }
    if (ncsi_fsm_retry_test(network_debug)) {
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_BEGIN);
    } else {
      ncsi_fsm_fail(ncsi_state, network_debug);
    }
    break;
  case NCSI_STATE_TEST_END:
    network_debug->ncsi.test.tries = 0;
    if (network_debug->ncsi.pending_restart) {
      ncsi_fsm_fail(ncsi_state, network_debug); // (Ab)use fail to restart.
    }
    if (++ncsi_state->retest_delay_count >= NCSI_FSM_RETEST_DELAY_COUNT) {
      GO_TO_STATE(state_variable, NCSI_STATE_TEST_BEGIN);
      ncsi_state->retest_delay_count = 0;
    }
    break;
  default:
    ncsi_fsm_fail(ncsi_state, network_debug);
    break;
  }

  ncsi_buf->len = len;
  return ncsi_response_type;
}
