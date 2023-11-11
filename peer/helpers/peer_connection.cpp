#include "peer_connection.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

void add_ip_table_entry(PORT port)
{
    std::string s = std::string("sudo iptables -A OUTPUT -p tcp -m tcp --sport ") + std::to_string(port.get_port(ByteOrder::HOST)) + std::string(" --tcp-flags RST RST -j DROP");
    
    if (fork() == 0)
    {
        std::clog << "Executing: " << s << std::endl;
        system(s.c_str());
        exit(0);
    }
    wait(NULL);
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