#include "ncsi_state_machine.h"

#include "common_defs.h"
#include "platforms/nemora/portable/default_addresses.h"
#include "platforms/nemora/portable/ncsi_fsm.h"

#include <arpa/inet.h>
#include <netinet/ether.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#define ETHER_NCSI 0x88f8

#define CPRINTF(...) fprintf(stderr, __VA_ARGS__)

#ifdef NCSID_VERBOSE_LOGGING
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...)
#endif

namespace ncsi
{

namespace
{

const char kStateFormat[] = "l2_config=%d/%d l3l4_config=%d/%d test=%d/%d";
// This assumes that the number of states is < 100, so each number
// in the format above does not take more than two characters to represent,
// thus %d (two characters) is substituted for the number which is also
// two characters max.
constexpr auto kStateFormatLen = sizeof(kStateFormat);

void snprintf_state(char* buffer, int size, const ncsi_state_t* state)
{
    (void)snprintf(buffer, size, kStateFormat, state->l2_config_state,
                   NCSI_STATE_L2_CONFIG_END, state->l3l4_config_state,
                   NCSI_STATE_L3L4_CONFIG_END, state->test_state,
                   NCSI_STATE_TEST_END);
}

void print_state(const ncsi_state_t& state)
{
    (void)state;
    DEBUG_PRINTF(kStateFormat, state.l2_config_state, NCSI_STATE_L2_CONFIG_END,
                 state.l3l4_config_state, NCSI_STATE_L3L4_CONFIG_END,
                 state.test_state, NCSI_STATE_TEST_END);
    DEBUG_PRINTF(" restart_delay_count=%d\n", state.restart_delay_count);
}

const uint8_t echo_pattern[NCSI_OEM_ECHO_PATTERN_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A,
    0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0x12, 0x34, 0x56, 0x78,
    0x9A, 0xBC, 0xDE, 0xF0, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};

} // namespace

void StateMachine::reset()
{
    std::memset(&ncsi_state_, 0, sizeof(ncsi_state_));
    ncsi_state_.restart_delay_count = NCSI_FSM_RESTART_DELAY_COUNT - 1;
    network_debug_.ncsi.test.max_tries = MAX_TRIES;
    // This needs to be initialized in the firmware.
    network_debug_.ncsi.test.ch_under_test = 0;
    network_debug_.ncsi.oem_filter_disable = false;

    network_debug_.ncsi.pending_stop = false;
    network_debug_.ncsi.enabled = true;
    network_debug_.ncsi.loopback = false;
}

StateMachine::StateMachine()
{
    reset();
    network_debug_.ncsi.pending_restart = true;
    std::memcpy(network_debug_.ncsi.test.ping.tx, echo_pattern,
                sizeof(echo_pattern));
}

size_t StateMachine::poll_l2_config()
{
    size_t len = 0;
    mac_addr_t mac;
    net_config_->get_mac_addr(&mac);
    ncsi_response_type_t response_type = ncsi_fsm_poll_l2_config(
        &ncsi_state_, &network_debug_, &ncsi_buf_, &mac);

    auto* response = reinterpret_cast<ncsi_simple_response_t*>(ncsi_buf_.data);

    if (response_type == NCSI_RESPONSE_ACK)
    {
        /* If the response is MAC response, some extra handling needed. */
        if ((NCSI_RESPONSE | NCSI_OEM_COMMAND) ==
            response->hdr.control_packet_type)
        {
            auto* oem_response =
                reinterpret_cast<ncsi_oem_simple_response_t*>(ncsi_buf_.data);
            if (oem_response->oem_header.oem_cmd ==
                NCSI_OEM_COMMAND_GET_HOST_MAC)
            {
                net_config_->set_mac_addr(mac);
            }
        }
    }
    else if (NCSI_RESPONSE_NONE == response_type)
    {
        /* Buffer is ready to be sent. */
        len = ncsi_buf_.len;
        ncsi_buf_.len = 0;
    }
    else
    {
        report_ncsi_error(response_type);
    }

    return len;
}

size_t StateMachine::poll_simple(ncsi_simple_poll_f poll_func)
{
    mac_addr_t mac;
    net_config_->get_mac_addr(&mac);
    const uint16_t rx_port = DEFAULT_ADDRESSES_RX_PORT;

    ncsi_response_type_t response_type =
        poll_func(&ncsi_state_, &network_debug_, &ncsi_buf_, &mac, 0, rx_port);

    auto* response = reinterpret_cast<ncsi_simple_response_t*>(ncsi_buf_.data);

    size_t len = 0;
    if (response_type == NCSI_RESPONSE_NONE)
    {
        /* Buffer is ready to be sent, or we are done. */
        len = ncsi_buf_.len;
        ncsi_buf_.len = 0;
    }
    else if (response->hdr.control_packet_type ==
             (NCSI_RESPONSE | NCSI_GET_LINK_STATUS))
    {
        auto status_response =
            reinterpret_cast<ncsi_link_status_response_t*>(response);
        bool new_link_up = ntohl(status_response->link_status.link_status) &
                           NCSI_LINK_STATUS_UP;
        if (!link_up_ || new_link_up != *link_up_)
        {
            CPRINTF("[NCSI link %s]\n", new_link_up ? "up" : "down");
            link_up_ = new_link_up;
        }
    }
    else if (response->hdr.control_packet_type ==
             (NCSI_RESPONSE | NCSI_OEM_COMMAND))
    {
        auto* oem_response =
            reinterpret_cast<ncsi_oem_simple_response_t*>(ncsi_buf_.data);
        if (oem_response->oem_header.oem_cmd == NCSI_OEM_COMMAND_GET_FILTER)
        {
            bool new_hostless = ncsi_fsm_is_nic_hostless(&ncsi_state_);
            if (!hostless_ || new_hostless != *hostless_)
            {
                CPRINTF("[NCSI nic %s]\n",
                        new_hostless ? "hostless" : "hostfull");
                net_config_->set_nic_hostless(new_hostless);
                hostless_ = new_hostless;
            }
        }
    }
    else if (response_type != NCSI_RESPONSE_ACK)
    {
        report_ncsi_error(response_type);
    }

    return len;
}

void StateMachine::report_ncsi_error(ncsi_response_type_t response_type)
{
    char state_string[kStateFormatLen];
    snprintf_state(state_string, sizeof(state_string), &ncsi_state_);
    auto* response =
        reinterpret_cast<const ncsi_simple_response_t*>(ncsi_buf_.data);
    switch (response_type)
    {
        case NCSI_RESPONSE_UNDERSIZED:
            if (!ncsi_buf_.len)
            {
                network_debug_.ncsi.rx_error.timeout_count++;
                CPRINTF("[NCSI timeout in state %s]\n", state_string);
            }
            else
            {
                network_debug_.ncsi.rx_error.undersized_count++;
                CPRINTF("[NCSI undersized response in state %s]\n",
                        state_string);
            }
            break;
        case NCSI_RESPONSE_NACK:
            network_debug_.ncsi.rx_error.nack_count++;
            CPRINTF(
                "[NCSI nack in state %s. Response: 0x%04x Reason: 0x%04x]\n",
                state_string, ntohs(response->response_code),
                ntohs(response->reason_code));
            break;
        case NCSI_RESPONSE_UNEXPECTED_TYPE:
            network_debug_.ncsi.rx_error.unexpected_type_count++;
            CPRINTF("[NCSI unexpected response in state %s. Response type: "
                    "0x%02x]\n",
                    state_string, response->hdr.control_packet_type);
            break;
        case NCSI_RESPONSE_UNEXPECTED_SIZE:
        {
            int expected_size;
            if ((NCSI_RESPONSE | NCSI_OEM_COMMAND) ==
                response->hdr.control_packet_type)
            {
                auto* oem_response =
                    reinterpret_cast<ncsi_oem_simple_response_t*>(
                        ncsi_buf_.data);
                expected_size = ncsi_oem_get_response_size(
                    oem_response->oem_header.oem_cmd);
            }
            else
            {
                expected_size = ncsi_get_response_size(
                    response->hdr.control_packet_type & (~NCSI_RESPONSE));
            }
            network_debug_.ncsi.rx_error.unexpected_size_count++;
            CPRINTF("[NCSI unexpected response size in state %s."
                    " Expected %d]\n",
                    state_string, expected_size);
        }
        break;
        case NCSI_RESPONSE_OEM_FORMAT_ERROR:
            network_debug_.ncsi.rx_error.unexpected_type_count++;
            CPRINTF("[NCSI OEM format error]\n");
            break;
        case NCSI_RESPONSE_UNEXPECTED_PARAMS:
            CPRINTF("[NCSI OEM Filter MAC or TCP/IP Config Mismatch]\n");
            break;
        default:
            /* NCSI_RESPONSE_ACK and NCSI_RESPONSE_NONE are not errors and need
             * not be handled here, so this branch is just to complete the
             * switch.
             */
            CPRINTF("[NCSI OK]\n");
            break;
    }
}

int StateMachine::receive_ncsi()
{
    int len;
    do
    {
        // Return value of zero means timeout
        len = sock_io_->recv(ncsi_buf_.data, sizeof(ncsi_buf_.data));
        if (len > 0)
        {
            auto* hdr = reinterpret_cast<struct ether_header*>(ncsi_buf_.data);
            if (ETHER_NCSI == ntohs(hdr->ether_type))
            {
                ncsi_buf_.len = len;
                break;
            }
        }

        ncsi_buf_.len = 0;
    } while (len > 0);

    return ncsi_buf_.len;
}

void StateMachine::run_test_fsm(size_t* tx_len)
{
    // Sleep and restart when test FSM finishes.
    if (is_test_done())
    {
        std::this_thread::sleep_for(std::chrono::seconds(retest_delay_s_));
        // Skip over busy wait in state machine - already waited.
        ncsi_state_.retest_delay_count = NCSI_FSM_RESTART_DELAY_COUNT;
    }
    // until NCSI_STATE_TEST_END
    *tx_len = poll_simple(ncsi_fsm_poll_test);
}

void StateMachine::run(int max_rounds)
{
    if (!net_config_ || !sock_io_)
    {
        CPRINTF("StateMachine configuration incomplete: "
                "net_config_: <%p>, sock_io_: <%p>",
                reinterpret_cast<void*>(net_config_),
                reinterpret_cast<void*>(sock_io_));
        return;
    }

    bool infinite_loop = (max_rounds <= 0);
    while (infinite_loop || --max_rounds >= 0)
    {
        receive_ncsi();

        size_t tx_len = 0;
        switch (ncsi_fsm_connection_state(&ncsi_state_, &network_debug_))
        {
            case NCSI_CONNECTION_DOWN:
            case NCSI_CONNECTION_LOOPBACK:
                tx_len = poll_l2_config();
                break;
            case NCSI_CONNECTION_UP:
                if (!is_test_done() || ncsi_fsm_is_nic_hostless(&ncsi_state_))
                {
                    run_test_fsm(&tx_len);
                }
                else
                {
                    // - Only start L3/L4 config when test is finished
                    // (it will last until success
                    // (i.e. NCSI_CONNECTION_UP_AND_CONFIGURED) or fail.
                    tx_len = poll_simple(ncsi_fsm_poll_l3l4_config);
                }
                break;
            case NCSI_CONNECTION_UP_AND_CONFIGURED:
                run_test_fsm(&tx_len);
                break;
            case NCSI_CONNECTION_DISABLED:
                if (network_debug_.ncsi.pending_restart)
                {
                    network_debug_.ncsi.enabled = true;
                }
                break;
            default:
                fail();
        }

        if (tx_len)
        {
            print_state(ncsi_state_);

            sock_io_->write(ncsi_buf_.data, tx_len);
        }
    }
}

void StateMachine::clear_state()
{
    // This implicitly resets:
    //   l2_config_state   to NCSI_STATE_L2_CONFIG_BEGIN
    //   l3l4_config_state to NCSI_STATE_L3L4_CONFIG_BEGIN
    //   test_state        to NCSI_STATE_TEST_BEGIN
    std::memset(&ncsi_state_, 0, sizeof(ncsi_state_));
}

void StateMachine::fail()
{
    network_debug_.ncsi.fail_count++;
    clear_state();
}

void StateMachine::set_sockio(net::SockIO* sock_io)
{
    sock_io_ = sock_io;
}

void StateMachine::set_net_config(net::ConfigBase* net_config)
{
    net_config_ = net_config;
}

void StateMachine::set_retest_delay(unsigned delay)
{
    retest_delay_s_ = delay;
}

} // namespace ncsi
