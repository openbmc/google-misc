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
#include "ncsi_sockio.h"
#include "net_config.h"
#include "net_iface.h"
#include "platforms/nemora/portable/ncsi_client.h"
#include "platforms/nemora/portable/ncsi_fsm.h"
#include "platforms/nemora/portable/net_types.h"

#include <optional>

namespace ncsi
{

typedef ncsi_response_type_t (*ncsi_simple_poll_f)(
    ncsi_state_t*, network_debug_t*, ncsi_buf_t*, mac_addr_t*, uint32_t,
    uint16_t);

// This class encapsulates three state machines:
//  * L2 -- performs basic NC-SI setup, reads NIC MAC addr
//  * L3/4 -- once network is configured on the interface,
//      sets up NC-SI filter in the NIC.
//  * Test -- runs several basic NC-SI link tests, like
//      ECHO Request/Reply, checks filter setup etc.
//      Also, reads hostless/host-based flag from the NIC, see
//      ncsi_fsm.c:is_nic_hostless() for details.
class StateMachine
{
  public:
    StateMachine();
    ~StateMachine();

    void set_sockio(net::SockIO* sock_io);

    void set_net_config(net::ConfigBase* net_config);

    // NC-SI State Machine's main function.
    // max_rounds = 0 means run forever.
    void run(int max_rounds = 0);

    // How often Test FSM re-runs, in seconds.
    void set_retest_delay(unsigned int delay);

  private:
    // Reset the state machine
    void reset();

    // Poll L2 state machine. Each call advances it by one step.
    // Its implementation is taken directly from EC.
    size_t poll_l2_config();

    // This function is used to poll both L3/4 and Test state machine,
    // depending on the function passed in as an argument.
    size_t poll_simple(ncsi_simple_poll_f poll_func);

    // Helper function for printing NC-SI error to stdout.
    void report_ncsi_error(ncsi_response_type_t response_type);

    int receive_ncsi();

    // Helper function for advancing the test FSM.
    void run_test_fsm(size_t* tx_len);

    // Clear the state and reset all state machines.
    void clear_state();

    // In current implementation this is the same as clear state,
    // except that it also increments the failure counter.
    void fail();

    // Return true if the test state machine finished successfully.
    bool is_test_done() const
    {
        return ncsi_state_.test_state == NCSI_STATE_TEST_END;
    }

    // Max number of times a state machine is going to retry a command.
    static constexpr auto MAX_TRIES = 5;

    // How long (in seconds) to wait before re-running NC-SI test state
    // machine.
    unsigned int retest_delay_s_ = 1;

    // The last known state of the link on the NIC
    std::optional<bool> link_up_;

    // The last known hostless mode of the NIC
    std::optional<bool> hostless_;

    // net_config_ is used to query and set network configuration.
    // The StateMachine does not own the pointer and it is the
    // responsibility of the client to make sure that it outlives the
    // StateMachine.
    net::ConfigBase* net_config_ = nullptr;

    // sock_io_ is used to read and write NC-SI packets.
    // The StateMachine does not own the pointer. It is the responsibility
    // of the client to make sure that sock_io_ outlives the StateMachine.
    net::SockIO* sock_io_ = nullptr;

    // Both ncsi_state_ and network_debug_ parameters represent the state of
    // the NC-SI state machine. The names and definitions are taken directly
    // from EC.
    ncsi_state_t ncsi_state_;
    network_debug_t network_debug_;

    // Depending on the state ncsi_buf_ represents either the NC-SI packet
    // received from the NIC or NC-SI packet that was (or about to be)
    // sent to the NIC.
    ncsi_buf_t ncsi_buf_;
};

} // namespace ncsi
