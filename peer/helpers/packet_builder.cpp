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
#include <thread>

using namespace PEER_CONNECTION;
const int BUFFER_SIZE = 65536;
enum class PacketDirection
{
    LOCAL_SERVER_TO_REMOTE_CLIENT,
    LOCAL_CLIENT_TO_REMOTE_SERVER,
    REMOTE_SERVER_TO_LOCAL_CLIENT,
    REMOTE_CLIENT_TO_LOCAL_SERVER
};

bool is_local_server_to_remote_client(ConnectionManager& cm, char* packet, int len)
{
    if (len < 40)
        return false;

    auto ip_header = (iphdr*)packet;
    auto iplen = (ip_header->ihl << 2);
    auto tcp_header = (tcphdr*)(packet + iplen);

    if (!cm.is_local_ip(ip_header->saddr) || !cm.is_local_ip(ip_header->daddr))
        return false;
    
    if (tcp_header->source != cm.config.self_server_port_n)
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
    
    if (cm.config.self_mimic_port_n != tcp_header->dest)
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
    return false;
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

void generate_packet(ConnectionManager& cm, void* source, PacketDirection pd, void* buff, int &len)
{

}

void packet_handler(ConnectionManager& cm)
{
    std::thread([&]()
    {
        // handle tcp raw socket here
        sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(sockaddr_in));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = cm.config.peer_port_n;
        peer_addr.sin_addr.s_addr = cm.config.peer_ip_n;

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

            int send_len = BUFFER_SIZE;
            if (is_local_server_to_remote_client(cm, receive_BUFF, dataSize))
                generate_packet(cm, receive_BUFF, PacketDirection::LOCAL_SERVER_TO_REMOTE_CLIENT, send_BUFF, send_len);
            else if (is_local_client_to_remote_server(cm, receive_BUFF, dataSize))
                generate_packet(cm, receive_BUFF, PacketDirection::LOCAL_CLIENT_TO_REMOTE_SERVER, send_BUFF, send_len);
            else continue;

            // send data as packet over udp
            socklen = sizeof(sockaddr_in);
            sendto(cm.peer_socket, send_BUFF, send_len, 0, (sockaddr*)&peer_addr, socklen);
        }
    }).detach();

    // handle udp peer socket here
    char receive_BUFF[BUFFER_SIZE];
    char send_BUFF[BUFFER_SIZE];

    while (true)
    {
        sockaddr_in sourceAddr;
        socklen_t socklen = sizeof(sockaddr_in);
        auto dataSize = recvfrom(cm.peer_socket, receive_BUFF, BUFFER_SIZE, 0, (struct sockaddr*)&sourceAddr, &socklen);
        if (dataSize < 0)
        {
            perror("tcp raw read");
            exit(EXIT_FAILURE);
        }

        if (sourceAddr.sin_port != cm.config.peer_port_n || sourceAddr.sin_addr.s_addr != cm.config.peer_ip_n)
            continue;

        int send_len = BUFFER_SIZE;
        if (is_remote_client_to_local_server(cm, receive_BUFF, dataSize))
            generate_packet(cm, receive_BUFF, PacketDirection::REMOTE_CLIENT_TO_LOCAL_SERVER, send_BUFF, send_len);
        else if (is_remote_server_to_local_client(cm, receive_BUFF, dataSize))
            generate_packet(cm, receive_BUFF, PacketDirection::REMOTE_SERVER_TO_LOCAL_CLIENT, send_BUFF, send_len);
        else continue;

        // send data as packet over raw tcp
        socklen = sizeof(sockaddr_in);
        continue;
        // TODO: What should be peer_addr here??????
        sendto(cm.raw_tcp_socket, send_BUFF, send_len, 0, nullptr, socklen);
    }
}