// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "net_iface_mock.h"
#include "nic_mock.h"
#include "platforms/nemora/portable/default_addresses.h"
#include "platforms/nemora/portable/ncsi.h"
#include "platforms/nemora/portable/ncsi_fsm.h"
#include "platforms/nemora/portable/net_types.h"

#include <ncsi_state_machine.h>
#include <net_config.h>
#include <net_sockio.h>
#include <netinet/ether.h>
#include <netinet/in.h>

#include <gmock/gmock.h>

namespace
{

constexpr uint32_t ETHER_NCSI = 0x88f8;

class MockConfig : public net::ConfigBase
{
  public:
    int get_mac_addr(mac_addr_t* mac) override
    {
        std::memcpy(mac, &mac_addr, sizeof(mac_addr_t));

        return 0;
    }

    int set_mac_addr(const mac_addr_t& mac) override
    {
        mac_addr = mac;

        return 0;
    }

    int set_nic_hostless(bool is_hostless) override
    {
        is_nic_hostless = is_hostless;

        return 0;
    }

    mac_addr_t mac_addr;
    bool is_nic_hostless = true;
};

class NICConnection : public net::SockIO
{
  public:
    int write(const void* buf, size_t len) override
    {
        conseq_reads = 0;
        ++n_writes;
        std::memcpy(last_write.data, buf, len);
        last_write.len = len;
        const auto* hdr = reinterpret_cast<const struct ether_header*>(buf);
        if (ETHER_NCSI == ntohs(hdr->ether_type))
        {
            ++n_handles;
            next_read.len = nic_mock.handle_request(last_write, &next_read);
        }

        return len;
    }

    int recv(void* buf, size_t maxlen) override
    {
        ++n_reads;
        ++conseq_reads;

        if (read_timeout > 0)
        {
            if (conseq_reads > read_timeout)
            {
                return 0;
            }
        }

        if (maxlen < next_read.len)
        {
            ++n_read_errs;
            return 0;
        }

        std::memcpy(buf, next_read.data, next_read.len);

        return next_read.len;
    }

    mock::NIC nic_mock{false, 2};
    int n_writes = 0;
    int n_reads = 0;
    int n_handles = 0;
    int n_read_errs = 0;

    // Max number of consequitive reads without writes.
    int read_timeout = -1;
    int conseq_reads = 0;

    ncsi_buf_t last_write = {};
    ncsi_buf_t next_read = {};
};

} // namespace

class TestNcsi : public testing::Test
{
  public:
    void SetUp() override
    {
        ncsi_sm.set_sockio(&ncsi_sock);
        ncsi_sm.set_net_config(&net_config_mock);
        ncsi_sm.set_retest_delay(0);
        ncsi_sock.nic_mock.set_mac(nic_mac);
        ncsi_sock.nic_mock.set_hostless(true);
        ncsi_sock.read_timeout = 10;
    }

  protected:
    void ExpectFiltersNotConfigured()
    {
        for (uint8_t i = 0; i < ncsi_sock.nic_mock.get_channel_count(); ++i)
        {
            EXPECT_FALSE(ncsi_sock.nic_mock.is_filter_configured(i));
        }
    }

    void ExpectFiltersConfigured()
    {
        // Check that filters are configured on all channels.
        for (uint8_t i = 0; i < ncsi_sock.nic_mock.get_channel_count(); ++i)
        {
            EXPECT_TRUE(ncsi_sock.nic_mock.is_filter_configured(i));
            const ncsi_oem_filter_t& ch_filter =
                ncsi_sock.nic_mock.get_filter(i);

            for (unsigned i = 0; i < sizeof(nic_mac.octet); ++i)
            {
                EXPECT_EQ(nic_mac.octet[i], ch_filter.mac[i]);
            }

            EXPECT_EQ(ch_filter.ip, 0);
            const uint16_t filter_port = ntohs(ch_filter.port);
            EXPECT_EQ(filter_port, DEFAULT_ADDRESSES_RX_PORT);
        }
    }

    MockConfig net_config_mock;
    NICConnection ncsi_sock;
    ncsi::StateMachine ncsi_sm;
    const mac_addr_t nic_mac = {{0xde, 0xca, 0xfb, 0xad, 0x01, 0x02}};

    // Number of states in each state machine
    static constexpr int l2_num_states = 26;
    static constexpr int l3l4_num_states = 2;
    static constexpr int test_num_states = 9;

    // Total number of states in all three state machines.
    static constexpr int total_num_states = l2_num_states + l3l4_num_states +
                                            test_num_states;
};

TEST_F(TestNcsi, TestMACAddrPropagation)
{
    ncsi_sm.run(total_num_states);
    EXPECT_EQ(ncsi_sock.n_read_errs, 0);
    EXPECT_EQ(ncsi_sock.n_handles, ncsi_sock.n_writes);
    EXPECT_EQ(0, std::memcmp(nic_mac.octet, net_config_mock.mac_addr.octet,
                             sizeof(nic_mac.octet)));

    // Since network is not configured, the filters should not be configured
    // either.
    ExpectFiltersNotConfigured();
}

TEST_F(TestNcsi, TestFilterConfiguration)
{
    ncsi_sm.run(total_num_states);
    EXPECT_EQ(ncsi_sock.n_read_errs, 0);
    EXPECT_EQ(ncsi_sock.n_handles, ncsi_sock.n_writes);

    ExpectFiltersConfigured();
}

TEST_F(TestNcsi, TestFilterReset)
{
    ncsi_sm.run(total_num_states);
    EXPECT_EQ(ncsi_sock.n_read_errs, 0);
    EXPECT_EQ(ncsi_sock.n_handles, ncsi_sock.n_writes);

    // Since network is not configured, the filters should not be configured
    // either.
    ExpectFiltersNotConfigured();

    ncsi_sm.run(total_num_states);

    ExpectFiltersConfigured();
}

TEST_F(TestNcsi, TestRetest)
{
    ncsi_sm.run(total_num_states + test_num_states);

    // Verify that the test state machine was stepped through twice,
    // by counting how many times the last command of the state machine
    // has been executed.
    const uint8_t last_test_command = NCSI_GET_LINK_STATUS;
    const auto& cmd_log = ncsi_sock.nic_mock.get_command_log();
    int num_test_runs = 0;
    for (const auto& ncsi_frame : cmd_log)
    {
        if (ncsi_frame.get_control_packet_type() == last_test_command)
        {
            ++num_test_runs;
        }
    }

    EXPECT_EQ(num_test_runs, 2);
}

TEST_F(TestNcsi, TestHostlessSwitch)
{
    // By default the NIC is in hostless mode.
    // Verify that net config flag changes after FSM run.
    net_config_mock.is_nic_hostless = false;
    ncsi_sm.run(total_num_states);
    EXPECT_EQ(ncsi_sock.n_read_errs, 0);
    EXPECT_EQ(ncsi_sock.n_handles, ncsi_sock.n_writes);
    EXPECT_TRUE(net_config_mock.is_nic_hostless);

    // Now disable the hostless mode and verify that net config
    // flag changes to false.
    ncsi_sock.nic_mock.set_hostless(false);
    ncsi_sm.run(total_num_states);
    EXPECT_EQ(ncsi_sock.n_read_errs, 0);
    EXPECT_EQ(ncsi_sock.n_handles, ncsi_sock.n_writes);
    EXPECT_FALSE(net_config_mock.is_nic_hostless);
}
