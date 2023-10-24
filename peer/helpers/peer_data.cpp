#include "peer_data.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/param.h>

#include <iostream>
#include <string>
#include <iomanip>
#include <thread>
#include <mutex>

const int BUFFER_SIZE = 65536;
/*
 * Helper functions
*/
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

bool is_valid_protocol(uint8_t protocol)
{
    return protocol == IPPROTO_TCP || protocol == IPPROTO_UDP;
}

const tcphdr* get_tcp_header(const iphdr* ip_header)
{
    int offset = (ip_header->ihl * 4) + (ip_header->protocol == IPPROTO_TCP ? 0 : 8);
    return (tcphdr*)((char*)ip_header + offset);
}

const udphdr* get_udp_header(const iphdr* ip_header)
{
    return ip_header->protocol == IPPROTO_TCP ? nullptr : (udphdr*)((char*)ip_header + ip_header->ihl * 4);
}

/*
 * ConnectionManager implementation
*/
bool ConnectionManager::is_local_ip(uint32_t ip) const
{
    for (auto &ipn: this->local_ips)
        if (ipn == ip)
            return true;
    return false;
}

ConnectionManager::ConnectionManager(const CONFIG::Config& config, int peer_socket) :
    config { config },
    local_ips { get_local_ips() },
    peer_socket { peer_socket }
{
    if (local_ips.empty())
        throw std::runtime_error("No IP address assigned to any interface");
    
    std::cout << std::endl;
    for (auto &ipn: local_ips)
        std::cout << "> Local IP: " << inet_ntoa({ ipn }) << std::endl;
}

PacketDirection ConnectionManager::get_packet_direction(const iphdr* ip_header, int len) const
{
    if (!is_valid_protocol(ip_header->protocol))
        return PacketDirection::UNKNOWN;

    auto source_ip = ip_header->saddr;
    auto dest_ip = ip_header->daddr;

    auto tcp_header = get_tcp_header(ip_header);
    auto source_port = tcp_header->source;
    auto dest_port = tcp_header->dest;

    // IP Check
    if ((source_ip == 0) != (dest_ip == 0))
        return PacketDirection::UNKNOWN;

    if (source_ip != 0 && (!is_local_ip(source_ip) || !is_local_ip(dest_ip)))
        return PacketDirection::UNKNOWN;
    
    // Source: (UDP + (IP = 0)) || (TCP + (IP = LOCAL))
    bool is_udp = ip_header->protocol == IPPROTO_UDP;
    bool is_zero = (source_ip == 0);
    
    if (is_udp != is_zero)
        return PacketDirection::UNKNOWN;
    
    // Port Check
    if (is_udp)
    {
        // We need to filter only those udp packets which are coming from peer
        auto udp_header = get_udp_header(ip_header);
        if (udp_header->source != config.peer_port_n || udp_header->dest != config.self_udp_port_n)
            return PacketDirection::UNKNOWN;

        // These ports refer to TCP payload ports.
        if ((source_port == 0) == (dest_port == 0))
            return PacketDirection::UNKNOWN;
        
        // Exactly one of PORT is zero and one is non-zero.
        if (source_port != 0)
            // remote generated port. This is a request from client, so it may or may not be seen yet.
            // This must be entered by caller in map and reverse map.
            return PacketDirection::REMOTE_CLIENT_TO_LOCAL_SERVER;
        
        // dest_port != 0. This is local client port.
        // This means current is reply to request from local client. This should be present in existing connection.
        if (local_client_ports.find(dest_port) != local_client_ports.end())
            return PacketDirection::REMOTE_SERVER_TO_LOCAL_CLIENT;
            
        return PacketDirection::UNKNOWN;
    }
    
    // TCP
    // This must be a generated port in response to remote client connection.
    if (source_port == config.self_server_port_n && rev_remote_to_generated.find(dest_port) != rev_remote_to_generated.end())
        return PacketDirection::LOCAL_SERVER_TO_REMOTE_CLIENT;

    if (dest_port == config.self_mimic_port_n)
        return PacketDirection::LOCAL_CLIENT_TO_REMOTE_SERVER;

    return PacketDirection::UNKNOWN;
}

void ConnectionManager::add_ip_table_entry(uint32_t port_n)
{
    std::string s = std::string("sudo iptables -A OUTPUT -p tcp -m tcp --sport ") + std::to_string(ntohs(port_n)) + std::string(" --tcp-flags RST RST -j DROP");
    
    if (fork() == 0)
    {
        std::clog << "Executing: " << s << std::endl;
        system(s.c_str());
        exit(0);
    }
}

void handle_local_server_to_remote_client(const iphdr* ip_header, int len, const tcphdr* tcp_header, const udphdr* udp_header)
{

}

void handle_local_client_to_remote_server(const iphdr* ip_header, int len, const tcphdr* tcp_header, const udphdr* udp_header)
{

}

void handle_remote_client_to_local_server(const iphdr* ip_header, int len, const tcphdr* tcp_header, const udphdr* udp_header)
{

}

void handle_remote_server_to_local_client(const iphdr* ip_header, int len, const tcphdr* tcp_header, const udphdr* udp_header)
{

}

void ConnectionManager::handle_packets(int tcp_raw_socket, int udp_raw_socket)
{
    auto handle_packets = [&](const iphdr* ip_header, const int packet_size)
    {
        auto direction = get_packet_direction(ip_header, packet_size);
        if (direction == PacketDirection::UNKNOWN)
            return;
        
        auto tcp_header = get_tcp_header(ip_header);
        auto udp_header = get_udp_header(ip_header);
        if (direction == PacketDirection::LOCAL_SERVER_TO_REMOTE_CLIENT)
            handle_local_server_to_remote_client(ip_header, packet_size, tcp_header, udp_header);
        else if (direction == PacketDirection::LOCAL_CLIENT_TO_REMOTE_SERVER)
            handle_local_client_to_remote_server(ip_header, packet_size, tcp_header, udp_header);
        else if (direction == PacketDirection::REMOTE_CLIENT_TO_LOCAL_SERVER)
            handle_remote_client_to_local_server(ip_header, packet_size, tcp_header, udp_header);
        else if (direction == PacketDirection::REMOTE_SERVER_TO_LOCAL_CLIENT)
            handle_remote_server_to_local_client(ip_header, packet_size, tcp_header, udp_header);
    };

    std::mutex _mtx;

    while (true)
    {
        std::thread([&]()
        {
            char buffer[BUFFER_SIZE];
            struct sockaddr_in source_addr;
            socklen_t addr_size = sizeof(source_addr);
            int packet_size = recvfrom(tcp_raw_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&source_addr, &addr_size);
            if (packet_size < 0)
            {
                perror("udp raw packet receive");
                close(tcp_raw_socket);
                exit(EXIT_FAILURE);
            }
            _mtx.lock();
            handle_packets((iphdr*)buffer, packet_size);
            _mtx.unlock();
        }).detach();

        
        char buffer[BUFFER_SIZE];
        struct sockaddr_in source_addr;
        socklen_t addr_size = sizeof(source_addr);
        int packet_size = recvfrom(tcp_raw_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&source_addr, &addr_size);
        if (packet_size < 0)
        {
            perror("tcp raw packet receive");
            close(tcp_raw_socket);
            exit(EXIT_FAILURE);
        }
        
        _mtx.lock();
        handle_packets((iphdr*)buffer, packet_size);
        _mtx.unlock();
    }
}