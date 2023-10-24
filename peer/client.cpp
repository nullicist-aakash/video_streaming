#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 65536
void compute_tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
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

int main() {
    int raw_socket;
    struct sockaddr_in source_addr;
    socklen_t addr_size = sizeof(source_addr);
    unsigned char buffer[BUFFER_SIZE];

    // Create a raw socket
    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_socket < 0) {
        perror("Socket creation error");
        return 1;
    }

    while (1) {
        int packet_size = recvfrom(raw_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&source_addr, &addr_size);
        if (packet_size < 0) {
            perror("Packet receive error");
            close(raw_socket);
            return 1;
        }

        struct iphdr* ip_header = (struct iphdr*)buffer;
        if (ip_header->protocol == IPPROTO_TCP) {
            struct tcphdr* tcp_header = (struct tcphdr*)(buffer + ip_header->ihl * 4);
            if (ntohs(tcp_header->dest) != 8080 && (ntohs(tcp_header->dest) != 8081)) {
                continue;
            }
            
            printf("Source Checksum: %02X\n", ntohs(tcp_header->check));
            compute_tcp_checksum(ip_header, (unsigned short*)tcp_header);
            printf("Computed Checksum: %02X\n", ntohs(tcp_header->check));
        }
    }

    close(raw_socket);
    return 0;
}