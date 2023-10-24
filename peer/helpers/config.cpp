#include "config.h"
#include <arpa/inet.h>

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    os << "> Relay IP          " << inet_ntoa(*(in_addr*)&config.relay_ip_n) << std::endl;
    os << "> Relay Port        " << ntohs(config.relay_port_n) << std::endl;
    os << std::endl;
    os << "> Self UDP Port     " << ntohs(config.self_udp_port_n) << std::endl;
    os << "> Self Server Port  " << ntohs(config.self_server_port_n) << std::endl;
    os << "> Remote Mimic Port " << ntohs(config.remote_mimic_port_n) << std::endl;
    os << std::endl;
    os << "> Identifier        " << config.identifier << std::endl;
    os << std::endl;
    os << "> Peer IP           " << inet_ntoa(*(in_addr*)&config.peer_ip_n) << std::endl;
    os << "> Peer Port         " << ntohs(config.peer_port_n) << std::endl;
    return os;
}