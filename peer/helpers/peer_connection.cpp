#include "peer_connection.h"
#include <iostream>
#include <string.h>
#include <thread>
#include <vector>

enum class ConnectionState
{
    STARTED,
    SENT_SELF,
    RECEIVED_PEER
};

namespace PEER_CONNECTION
{
    void add_ip_table_entry(PORT port)
    {
        std::string s = std::string("sudo iptables -A OUTPUT -p tcp -m tcp --sport ") + std::to_string(port.get_port(ByteOrder::HOST)) + std::string(" --tcp-flags RST RST -j DROP");
        
        if (fork() == 0)
        {
            std::clog << "Executing: " << s << std::endl;
            system(s.c_str());
            exit(0);
        }
    }

    Socket get_peer_from_relay(const UDP& udp, CONFIG::Config& config)
    {
        ConnectionState state = ConnectionState::STARTED;

        auto thread = std::thread([&]()
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                udp.send(config.identifier, config.relay_info);
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });

        Socket pa;
        while (true)
        {
            auto [str, sock] = udp.receive();

            if ((str.size() != sizeof(pa)) || sock != config.relay_info)
                continue;

            memcpy(&pa, str.c_str(), sizeof(pa));
            break;
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
        return pa;
    }

    void make_connection_with_peer(const UDP& udp, CONFIG::Config& config)
    {
        ConnectionState state = ConnectionState::STARTED;
        
        // send messages in a loop to peer, untill we receive something from other side
        auto thread = std::thread([&]() 
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                udp.send(config.identifier, config.peer_info);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        while (true)
        {
            auto [str, sock] = udp.receive();

            if (str != config.identifier || config.peer_info != sock)
                continue;
                
            std::cout << "Received from peer: " << str << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            break;
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
    }

    UDP get_peer_udp(CONFIG::Config& config)
    {
        UDP udp(config.self_udp_port);
        
        if (config.peer_info.ip == IP(0) || config.peer_info.port == PORT(0) || config.self_udp_port == PORT(0))
        {
            config.peer_info = get_peer_from_relay(udp, config);
            config.self_udp_port = udp.get_self_port();
        }

        make_connection_with_peer(udp, config);

        return udp;
    }

    ConnectionManager::ConnectionManager(
        const CONFIG::Config& config, UDP&& peer_socket,
        int raw_tcp_socket) :
        config { config },
        local_ips { IP::get_local_ips() },
        peer_socket { std::move(peer_socket) },
        raw_tcp_socket { raw_tcp_socket }
    {
        if (local_ips.empty())
            throw std::runtime_error("No IP address assigned to any interface");
        
        std::clog << std::endl;
        for (auto &ip: local_ips)
            std::clog << "> Local IP: " << ip << std::endl;
        
        // add ip table entry for all mimic port
        add_ip_table_entry(config.self_mimic_port);
    }

    bool ConnectionManager::is_local_ip(IP ip) const
    {
        for (auto &ipn: this->local_ips)
            if (ipn == ip)
                return true;
        return false;
    }

    PORT ConnectionManager::get_generated_port(PORT port_n)
    {
        if (this->remote_to_generated.find(port_n) != this->remote_to_generated.end())
            return this->remote_to_generated[port_n];

        this->remote_to_generated[port_n] = this->generated_port_counter;
        this->rev_remote_to_generated[this->generated_port_counter] = port_n;
        add_ip_table_entry(port_n);
        return this->generated_port_counter++;
    }

    PORT ConnectionManager::get_remote_port_from_generated(PORT port_n)
    {
        if (is_local_generated_port(port_n))
            return this->rev_remote_to_generated[port_n];
        return 0;
    }

    void ConnectionManager::add_local_client_port(PORT port_n)
    {
        this->local_client_ports.insert(port_n);
    }

    bool ConnectionManager::is_local_client_port(PORT port_n)
    {
        return this->local_client_ports.find(port_n) != this->local_client_ports.end();
    }

    bool ConnectionManager::is_remote_client_port(PORT port_n)
    {
        return this->remote_to_generated.find(port_n) != this->remote_to_generated.end();
    }

    bool ConnectionManager::is_local_generated_port(PORT port_n)
    {
        return this->rev_remote_to_generated.find(port_n) != this->rev_remote_to_generated.end();
    }
}