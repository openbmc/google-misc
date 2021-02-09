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
