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
    struct SocketPair
    {
        uint32_t self_ip;
        uint32_t peer_ip;
        uint16_t self_port;
        uint16_t peer_port;
    };

    class ConnectionManager
    {
        // This should not be named as generated_to_remote due to convention
        std::map<int, int> rev_remote_to_generated;
        std::map<int, int> remote_to_generated;
        std::set<int> local_client_ports;
        int generated_port_counter = 8084;

        void add_ip_table_entry(uint32_t port_n);
    public:
        const int peer_socket;
        const int raw_tcp_socket;
        const CONFIG::Config config;
        const std::vector<uint32_t> local_ips;

        ConnectionManager(const CONFIG::Config& config, int peer_socket, int raw_tcp_socket);
        bool is_local_ip(uint32_t) const;
        uint32_t get_generated_port(uint32_t);
        uint32_t get_remote_port_from_generated(uint32_t);
        void add_local_client_port(uint32_t);
        bool is_local_client_port(uint32_t);
        bool is_remote_client_port(uint32_t);
        bool is_local_generated_port(uint32_t);
    };

    SocketPair get_pa_from_relay(int, const sockaddr_in&, const char*);
    void make_connection_with_peer(int, const char*, const SocketPair&);
    int get_peer_udp(CONFIG::Config&);
}