#pragma once
#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <set>
#include <vector>

namespace PEER_CONNECTION
{
    class PORTComparator
    {
    public:
        bool operator()(const PORT& lhs, const PORT& rhs) const
        {
            return lhs.get_port() < rhs.get_port();
        }
    };

    class ConnectionManager
    {
        // This should not be named as generated_to_remote due to convention
        std::map<PORT, PORT, PORTComparator> rev_remote_to_generated;
        std::map<PORT, PORT, PORTComparator> remote_to_generated;
        std::set<PORT, PORTComparator> local_client_ports;
        int generated_port_counter = 8084;

    public:
        const int peer_socket;
        const int raw_tcp_socket;
        const CONFIG::Config config;
        const std::vector<IP> local_ips;

        ConnectionManager(const CONFIG::Config& config, int peer_socket, int raw_tcp_socket);
        bool is_local_ip(IP) const;
        PORT get_generated_port(PORT);
        PORT get_remote_port_from_generated(PORT);
        void add_local_client_port(PORT);
        bool is_local_client_port(PORT);
        bool is_remote_client_port(PORT);
        bool is_local_generated_port(PORT);
    };

    Socket get_pa_from_relay(int, const sockaddr_in&, const char*);
    void make_connection_with_peer(int, const char*, const Socket&);
    int get_peer_udp(CONFIG::Config&);
}