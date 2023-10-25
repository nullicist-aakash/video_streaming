#include "peer_connection.h"
#include <iostream>
#include <string.h>
#include <thread>
#include <vector>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/param.h>

enum class ConnectionState
{
    STARTED,
    SENT_SELF,
    RECEIVED_PEER
};

namespace PEER_CONNECTION
{
    void add_ip_table_entry(uint32_t port_n)
    {
        std::string s = std::string("sudo iptables -A OUTPUT -p tcp -m tcp --sport ") + std::to_string(ntohs(port_n)) + std::string(" --tcp-flags RST RST -j DROP");
        
        if (fork() == 0)
        {
            std::clog << "Executing: " << s << std::endl;
            system(s.c_str());
            exit(0);
        }
    }

    SocketPair get_pa_from_relay(int relay_socket, const sockaddr_in& relay_addr, const char* identifier)
    {
        auto identifier_len = strlen(identifier);
        ConnectionState state = ConnectionState::STARTED;

        auto thread = std::thread([&]() 
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                sendto(relay_socket, identifier, identifier_len, 0, (sockaddr*)&relay_addr, sizeof(relay_addr));
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });

        SocketPair pa;
        while (true)
        {
            int n = recvfrom(relay_socket, &pa, sizeof(pa), 0, NULL, NULL);
            if (n < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            if (n == sizeof(pa) && pa.peer_ip != 0 && pa.peer_port != 0)
                break;
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
        return pa;
    }

    void make_connection_with_peer(int relay_socket, const char* identifier, const SocketPair& pa)
    {
        sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(peer_addr));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = pa.peer_port;
        peer_addr.sin_addr.s_addr = pa.peer_ip;

        ConnectionState state = ConnectionState::STARTED;
        auto identifier_len = strlen(identifier);

        // send messages in a loop to peer, untill we receive something from other side
        auto thread = std::thread([&]() 
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                sendto(relay_socket, identifier, identifier_len, 0, (sockaddr*)&peer_addr, sizeof(peer_addr));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        
        while (true)
        {
            char BUFF[strlen(identifier) + 1];
            memset(BUFF, 0, sizeof(BUFF));

            if (recvfrom(relay_socket, BUFF, sizeof(BUFF), 0, NULL, NULL) < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            if (strcmp(BUFF, identifier) == 0 && pa.peer_ip == peer_addr.sin_addr.s_addr && pa.peer_port == peer_addr.sin_port)
            {
                std::cout << "Received from peer: " << BUFF << std::endl;
                break;
            }
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
    }

    int get_peer_udp(CONFIG::Config& config)
    {
        auto relay_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (relay_socket < 0)
        {
            perror("udp socket");
            exit(EXIT_FAILURE);
        }

        if (config.self_udp_port_n > 0)
        {
            struct sockaddr_in selfAddr;
            memset(&selfAddr, 0, sizeof(selfAddr));
            selfAddr.sin_family = AF_INET;
            selfAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            selfAddr.sin_port = config.self_udp_port_n;
        
            if (bind(relay_socket, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) == -1)
            {
                perror("bind_listen");
                exit(EXIT_FAILURE);
            }
        }

        sockaddr_in relay_addr;
        memset(&relay_addr, 0, sizeof(sockaddr_in));
        relay_addr.sin_family = AF_INET;
        relay_addr.sin_port = config.relay_port_n;
        relay_addr.sin_addr.s_addr = config.relay_ip_n;
        
        SocketPair pa;

        if (config.peer_ip_n == 0 || config.peer_port_n == 0 || config.self_udp_port_n == 0)
            pa = get_pa_from_relay(relay_socket, relay_addr, config.identifier.c_str());
        else
        {
            pa.peer_ip = config.peer_ip_n;
            pa.peer_port = config.peer_port_n;
        }
        
        make_connection_with_peer(relay_socket, config.identifier.c_str(), pa);
        config.peer_ip_n = pa.peer_ip;
        config.peer_port_n = pa.peer_port;
        config.self_udp_port_n = pa.self_port;

        return relay_socket;
    }

    std::vector<uint32_t> get_local_ips() 
    {
        ifaddrs* ifAddrStruct = nullptr;
        getifaddrs(&ifAddrStruct);
        if (ifAddrStruct == nullptr)
            return {};

        bool found = false;

        std::vector<uint32_t> ips;
        for (ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) 
        {
            if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET)
                continue;

            auto ipn = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
            if (ipn == 0 || ipn == 0xFFFFFFFF)
                continue;
                
            ips.push_back(ipn);
        }

        freeifaddrs(ifAddrStruct);
        return ips;
    }

    ConnectionManager::ConnectionManager(
        const CONFIG::Config& config, int peer_socket,
        int raw_tcp_socket) :
        config { config },
        local_ips { get_local_ips() },
        peer_socket { peer_socket },
        raw_tcp_socket { raw_tcp_socket }
    {
        if (local_ips.empty())
            throw std::runtime_error("No IP address assigned to any interface");
        
        std::clog << std::endl;
        for (auto &ipn: local_ips)
            std::clog << "> Local IP: " << inet_ntoa({ ipn }) << std::endl;
        
        // add ip table entry for all mimic port
        add_ip_table_entry(config.self_mimic_port_n);
    }

    bool ConnectionManager::is_local_ip(uint32_t ip) const
    {
        for (auto &ipn: this->local_ips)
            if (ipn == ip)
                return true;
        return false;
    }

    uint32_t ConnectionManager::get_generated_port(uint32_t port_n)
    {
        if (this->remote_to_generated.find(port_n) != this->remote_to_generated.end())
            return this->remote_to_generated[port_n];

        this->remote_to_generated[port_n] = this->generated_port_counter;
        this->rev_remote_to_generated[this->generated_port_counter] = port_n;
        add_ip_table_entry(port_n);
        return this->generated_port_counter++;
    }

    uint32_t ConnectionManager::get_remote_port_from_generated(uint32_t port_n)
    {
        if (is_local_generated_port(port_n))
            return this->rev_remote_to_generated[port_n];
        return 0;
    }

    void ConnectionManager::add_local_client_port(uint32_t port_n)
    {
        this->local_client_ports.insert(port_n);
    }

    bool ConnectionManager::is_local_client_port(uint32_t port_n)
    {
        return this->local_client_ports.find(port_n) != this->local_client_ports.end();
    }

    bool ConnectionManager::is_remote_client_port(uint32_t port_n)
    {
        return this->remote_to_generated.find(port_n) != this->remote_to_generated.end();
    }

    bool ConnectionManager::is_local_generated_port(uint32_t port_n)
    {
        return this->rev_remote_to_generated.find(port_n) != this->rev_remote_to_generated.end();
    }
}