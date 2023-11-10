#pragma once
#include "config.h"
#include "../../network/connection_manager.h"
#include <map>
#include <set>
#include <vector>
using namespace Network;

class ConnectionManager
{
    // This should not be named as generated_to_remote due to convention
    std::map<PORT, PORT> rev_remote_to_generated;
    std::map<PORT, PORT> remote_to_generated;
    std::set<PORT> local_client_ports;
    int generated_port_counter = 8084;

public:
    const UDP peer_socket;
    const int raw_tcp_socket;
    const CONFIG::Config config;
    const std::vector<IP> local_ips;

    ConnectionManager(const CONFIG::Config& config, UDP&& peer_socket, int raw_tcp_socket);
    bool is_local_ip(IP) const;
    PORT get_generated_port(PORT);
    PORT get_remote_port_from_generated(PORT);
    void add_local_client_port(PORT);
    bool is_local_client_port(PORT);
    bool is_remote_client_port(PORT);
    bool is_local_generated_port(PORT);
};