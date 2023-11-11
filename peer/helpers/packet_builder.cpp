#include "packet_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <iomanip>

#define HEX( x, len ) std::setw(2 * len) << std::setfill('0') << std::hex << std::uppercase << (((1ll << (8 * len)) - 1) & (unsigned int)( x )) << std::dec

void hex_view(const char* str, const size_t len)
{
    for (int i = 0; i < ((int)len >> 4) + (len % 16 ? 1 : 0); ++i)
    {
        int start = i * 16;
        int end = std::min(i * 16 + 15, (int)len - 1);
        std::clog << "0x" << HEX(start, 4) << " |  ";
        for (int j = start; j <= end; ++j)
            std::clog << HEX(str[j], 1) << (j % 8 == 7 ? "  " : " ");

        for (int j = end; j < i * 16 + 15; ++j)
            std::clog << (j % 8 == 7 ? "    " : "   ");
        std::clog << "| ";
        for (int j = start; j <= end; ++j)
            std::clog << (std::isprint(str[j]) ? str[j] : '.');
        std::clog << '\n';
    }
}

const int BUFFER_SIZE = 65536;
enum class PacketDirection
{
    LOCAL_SERVER_TO_REMOTE_CLIENT,
    LOCAL_CLIENT_TO_REMOTE_SERVER,
    REMOTE_SERVER_TO_LOCAL_CLIENT,
    REMOTE_CLIENT_TO_LOCAL_SERVER
};

uint16_t compute_tcp_checksum(unsigned short* tcp_segment, int len, uint32_t saddr, uint32_t daddr) 
{
    unsigned long sum = 0;
    ((struct tcphdr*)tcp_segment)->check = 0;;

    // move past ports
    tcp_segment += 2;
    len -= 4;

    // the source ip
    sum += (saddr >> 16) & 0xFFFF;
    sum += (saddr) & 0xFFFF;

    // the dest ip
    sum += (daddr >> 16) & 0xFFFF;
    sum += (daddr) & 0xFFFF;

    // protocol and reserved: 6
    sum += htons(IPPROTO_TCP);

    // the length
    sum += htons(len);
 
    // add the IP payload
    // initialize checksum to 0
    while (len > 1) 
    {
        sum += *tcp_segment++;
        len -= 2;
    }

    if (len > 0) sum += ((*tcp_segment) & htons(0xFF00));

    //Fold 32-bit sum to 16 bits: add carrier to result
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    sum = ~sum;
    return sum;
}

bool is_local_server_to_remote_client(ConnectionManager& cm, char* packet, int len)
{
    if (len < 40)
        return false;

    auto ip_header = (iphdr*)packet;
    auto iplen = (ip_header->ihl << 2);
    auto tcp_header = (tcphdr*)(packet + iplen);

    if (!cm.is_local_ip(ip_header->saddr) || !cm.is_local_ip(ip_header->daddr))
        return false;
    
    if (tcp_header->source != cm.config.self_server_port.get_port())
        return false;
    
    return cm.is_local_generated_port(tcp_header->source);
}

bool is_local_client_to_remote_server(ConnectionManager& cm, char* packet, int len)
{
    if (len < 40)
        return false;

    auto ip_header = (iphdr*)packet;
    auto iplen = (ip_header->ihl << 2);
    auto tcp_header = (tcphdr*)(packet + iplen);

    if (!cm.is_local_ip(ip_header->saddr) || !cm.is_local_ip(ip_header->daddr))
        return false;
    
    if (cm.config.self_mimic_port.get_port() != tcp_header->dest)
        return false;

    cm.add_local_client_port(tcp_header->source);
    return true;
}

bool is_remote_client_to_local_server(ConnectionManager& cm, char* packet, int len)
{
    if (len < 20)
        return false;
    
    auto tcp_header = (tcphdr*)packet;

    if (tcp_header->dest != 0)
        return false;
    
    cm.get_generated_port(tcp_header->source);
    return true;
}

bool is_remote_server_to_local_client(ConnectionManager& cm, char* packet, int len)
{
    if (len < 20)
        return false;
    
    auto tcp_header = (tcphdr*)packet;

    if (tcp_header->source != 0)
        return false;
    
    return cm.is_local_client_port(tcp_header->dest);
}

void generate_udp_packet(ConnectionManager& cm, char* source, PacketDirection pd, char* buff, ssize_t &len)
{
    auto ip_header = (iphdr*)source;
    auto ip_len = (ip_header->ihl << 2);
    auto tcp_header = (tcphdr*)(source + ip_len);

    // Modify source and destination ports
    if (pd == PacketDirection::LOCAL_SERVER_TO_REMOTE_CLIENT)
    {
        tcp_header->source = 0;
        tcp_header->dest = cm.get_remote_port_from_generated(tcp_header->dest).get_port();
    }
    else if (pd == PacketDirection::LOCAL_CLIENT_TO_REMOTE_SERVER)
        tcp_header->dest = 0;
    else assert(false);

    // Reset checksum to 0 as no one will read it till remote end
    tcp_header->check = 0;

    // set length and copy contents
    
    len -= ip_len;
    memcpy(buff, tcp_header, len);
}

void generate_tcp_packet(ConnectionManager& cm, char* source, PacketDirection pd, void* buff, int len)
{
    memcpy(buff, source, len);

    // Change source and destination ports
    auto tcp_header = (tcphdr*)buff;
    if (pd == PacketDirection::REMOTE_CLIENT_TO_LOCAL_SERVER)
    {
        tcp_header->source = cm.get_generated_port(tcp_header->source).get_port();
        tcp_header->dest = cm.config.self_server_port.get_port();
    }
    else if (pd == PacketDirection::REMOTE_SERVER_TO_LOCAL_CLIENT)
        tcp_header->source = cm.config.self_mimic_port.get_port();
    else assert(false);
    
    // Compute new checksum
    uint32_t ip;
    inet_pton(AF_INET, "127.0.0.1", &ip); // Destination IP (adjust as needed)
    tcp_header->check = compute_tcp_checksum((unsigned short*)tcp_header, len, ip, ip);
}

void packet_handler(ConnectionManager& cm)
{
    std::thread([&]()
    {
        // handle tcp raw socket here
        sockaddr_in peer_addr = cm.config.peer_info;

        char receive_BUFF[BUFFER_SIZE];
        char send_BUFF[BUFFER_SIZE];
        while (true)
        {
            sockaddr_in sourceAddr;
            socklen_t socklen = sizeof(sockaddr_in);
            auto dataSize = recvfrom(cm.raw_tcp_socket, receive_BUFF, BUFFER_SIZE, 0, (struct sockaddr*)&sourceAddr, &socklen);
            if (dataSize < 0)
            {
                perror("tcp raw read");
                exit(EXIT_FAILURE);
            }

            std::clog << "[" << Socket{sourceAddr} << "]: Received via RAW" << std::endl;
            hex_view(receive_BUFF, dataSize);

            if (is_local_server_to_remote_client(cm, receive_BUFF, dataSize))
                generate_udp_packet(cm, receive_BUFF, PacketDirection::LOCAL_SERVER_TO_REMOTE_CLIENT, send_BUFF, dataSize);
            else if (is_local_client_to_remote_server(cm, receive_BUFF, dataSize))
                generate_udp_packet(cm, receive_BUFF, PacketDirection::LOCAL_CLIENT_TO_REMOTE_SERVER, send_BUFF, dataSize);
            else continue;

            // send data as packet over udp
            cm.peer_socket.send(std::string(send_BUFF, dataSize), cm.config.peer_info);
        }
    }).detach();

    // handle udp peer socket here
    while (true)
    {
        socklen_t socklen = sizeof(sockaddr_in);
        auto [msg, source_sock] = cm.peer_socket.receive();

        if (source_sock != cm.config.peer_info)
            continue;

        std::string send_BUFF(msg.size(), '\0');

        if (is_remote_client_to_local_server(cm, msg.data(), msg.size()))
            generate_tcp_packet(cm, msg.data(), PacketDirection::REMOTE_CLIENT_TO_LOCAL_SERVER, send_BUFF.data(), send_BUFF.size());
        else if (is_remote_server_to_local_client(cm, msg.data(), msg.size()))
            generate_tcp_packet(cm, msg.data(), PacketDirection::REMOTE_SERVER_TO_LOCAL_CLIENT, send_BUFF.data(), send_BUFF.size());
        else continue;

        // send data as packet over raw tcp socket
        sockaddr_in destAddr = Socket{IP{"127.0.0.1"}, PORT{((tcphdr*)send_BUFF.data())->dest, ByteOrder::NETWORK}};
        socklen = sizeof(sockaddr_in);
        sendto(cm.raw_tcp_socket, send_BUFF.data(), send_BUFF.size(), 0, (sockaddr*)&destAddr, socklen);
        
        std::clog << "[" << Socket{destAddr} << "]: Sending via RAW" << std::endl;
        hex_view(send_BUFF.data(), send_BUFF.size());
    }
}