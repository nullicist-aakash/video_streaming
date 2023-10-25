#include "packet_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <cassert>

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
    if (len < 48)
        return false;
    
    auto ip_header = (iphdr*)packet;
    auto iplen = (ip_header->ihl << 2);
    auto udp_header = (udphdr*)(packet + iplen);
    auto tcp_header = (tcphdr*)(packet + iplen + 8);

    if (ip_header->saddr != cm.config.peer_ip_n)
        return false;
    
    if (!cm.is_local_ip(ip_header->daddr))
        return false;
    
    if (udp_header->source != cm.config.peer_port_n)
        return false;
    
    if (udp_header->dest != cm.config.self_udp_port_n)
        return false;
    
    if (tcp_header->dest != 0)
        return false;
    
    cm.get_generated_port(tcp_header->dest);
    return false;
}

bool is_remote_server_to_local_client(ConnectionManager& cm, char* packet, int len)
{
    if (len < 48)
        return false;
    
    auto ip_header = (iphdr*)packet;
    auto iplen = (ip_header->ihl << 2);
    auto udp_header = (udphdr*)(packet + iplen);
    auto tcp_header = (tcphdr*)(packet + iplen + 8);

    if (ip_header->saddr != cm.config.peer_ip_n)
        return false;
    
    if (!cm.is_local_ip(ip_header->daddr))
        return false;
    
    if (udp_header->source != cm.config.peer_port_n)
        return false;
    
    if (udp_header->dest != cm.config.self_udp_port_n)
        return false;
    
    if (tcp_header->source != 0)
        return false;
    
    return cm.is_local_client_port(tcp_header->dest);
}

void packet_handler(ConnectionManager& cm)
{

}