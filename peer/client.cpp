#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <thread>
#include <mutex>
#include "peer_connection.h"

#define BUFFER_SIZE 65536

enum class PacketDirection
{
    TO_REMOTE,
    TO_LOCAL,
    DROP
};

class StateManager
{
private:
    const int local_server_port;
    const uint32_t local_server_ip;
public:
    StateManager(sockaddr_in local_server_addr) :
      local_server_port(ntohs(local_server_addr.sin_port)), 
      local_server_ip(ntohl(local_server_addr.sin_addr.s_addr)) 
    {}

    bool is_local(sockaddr_in* addr) const 
    {
        return ntohs(addr->sin_port) == local_server_port && ntohl(addr->sin_addr.s_addr) == local_server_ip;
    }

    PacketDirection get_packet_direction(const iphdr* ip_header, int len) const
    {
        sockaddr_in source_addr;
        sockaddr_in dest_addr;

        source_addr.sin_addr.s_addr = ip_header->saddr;
        dest_addr.sin_addr.s_addr = ip_header->daddr;

        int offset = (ip_header->ihl * 4) + (ip_header->protocol == IPPROTO_TCP ? 0 : 8);
        if (offset + 4 > len)
            return PacketDirection::DROP;
        
        tcphdr *tcp_header = (tcphdr*)(ip_header + offset);
        source_addr.sin_port = tcp_header->source;
        dest_addr.sin_port = tcp_header->dest;

        bool source_is_local = is_local(&source_addr);
        bool dest_is_local = is_local(&dest_addr);

        if (source_is_local && dest_is_local)
            return PacketDirection::DROP;

        return source_is_local ? PacketDirection::TO_REMOTE : PacketDirection::TO_LOCAL;
    }

    bool send_udp(const iphdr* ip_header, int len)
    {
        return false;
    }

    bool send_tcp(const iphdr* ip_header, int len)
    {
        return false;
    }
};

void compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) 
{
    unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2) - 4;
    struct tcphdr *tcphdrp = (struct tcphdr*)(ipPayload + 4);
    //add the pseudo header 
    //the source ip
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    //the dest ip
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    //protocol and reserved: 6
    sum += htons(IPPROTO_TCP);
    //the length
    sum += htons(tcpLen);
 
    //add the IP payload
    //initialize checksum to 0
    tcphdrp->check = 0;
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }
    //if any bytes left, pad the bytes and add
    if(tcpLen > 0) {
        //printf("+++++++++++padding, %dn", tcpLen);
        sum += ((*ipPayload)&htons(0xFF00));
    }
      //Fold 32-bit sum to 16 bits: add carrier to result
      while (sum>>16) {
          sum = (sum & 0xffff) + (sum >> 16);
      }
      sum = ~sum;
    //set computation result
    tcphdrp->check = (unsigned short)sum;
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <relay_ip> <relay_port> <identifier>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* relay_ip = argv[1];
    const int relay_port = atoi(argv[2]);
    const char* identifier = argv[3];
    
    auto peer_socket = PEER::get_peer_udp(relay_ip, relay_port, identifier);
    printf("Succesfully connected with PEER!\n");
    return 0;

    struct sockaddr_in source_addr;
    socklen_t addr_size = sizeof(source_addr);
    unsigned char buffer[BUFFER_SIZE];

    // Create a raw socket
    auto tcp_raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    auto udp_raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    if (udp_raw_socket < 0)
    {
        perror("UDP Socket creation error");
        return EXIT_FAILURE;
    }

    if (tcp_raw_socket < 0)
    {
        perror("TCP Socket creation error");
        return EXIT_FAILURE;
    }

    std::mutex _mtx;
    int raw_socket = 0;
    while (1)
    {
        int packet_size = recvfrom(raw_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&source_addr, &addr_size);
        if (packet_size < 0)
        {
            perror("Packet receive error");
            exit(EXIT_FAILURE);
        }

        struct iphdr* ip_header = (struct iphdr*)buffer;
        if (ip_header->protocol == IPPROTO_TCP)
        {
            struct tcphdr* tcp_header = (struct tcphdr*)(buffer + ip_header->ihl * 4);
            if (ntohs(tcp_header->dest) != 8080 && (ntohs(tcp_header->dest) != 8081))
                continue;
            
            printf("Source Checksum: %02X\n", ntohs(tcp_header->check));
            compute_tcp_checksum(ip_header, (unsigned short*)tcp_header);
            printf("Computed Checksum: %02X\n", ntohs(tcp_header->check));
        }
    }

    return 0;
}