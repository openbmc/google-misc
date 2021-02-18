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

#include <ncsi_sockio.h>
#include <ncsi_state_machine.h>
#include <net_config.h>

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <interface_name>" << std::endl;
        return -1;
    }

    std::string iface_name(argv[1]);

    net::PhosphorConfig net_config(iface_name);
    net::IFace eth(iface_name);

    ncsi::SockIO ncsi_sock;
    ncsi_sock.init();
    ncsi_sock.bind_to_iface(eth);
    ncsi_sock.filter_vlans();

    ncsi::StateMachine ncsi_fsm;
    ncsi_fsm.set_sockio(&ncsi_sock);
    ncsi_fsm.set_net_config(&net_config);

    // If run ever returns -- it's an error.
    ncsi_fsm.run();

    return -1;
}
