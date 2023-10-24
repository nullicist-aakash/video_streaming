#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 65536

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
            int payload_offset = ip_header->ihl * 4 + tcp_header->doff * 4;
            int payload_size = packet_size - payload_offset;

            // Print source and destination addresses and ports
            printf("Source IP: %s\n", inet_ntoa(source_addr.sin_addr));
            printf("Source Port: %d\n", ntohs(tcp_header->source));
            printf("Dest IP: %s\n", inet_ntoa(source_addr.sin_addr));
            printf("Dest Port: %d\n", ntohs(tcp_header->dest));

            // Print payload (if any)
            if (payload_size > 0) {
                printf("Payload:\n");
                for (int i = payload_offset; i < packet_size; i++) {
                    printf("%02X ", buffer[i]);
                }
                printf("\n");
            }
        }
    }

    close(raw_socket);
    return 0;
}