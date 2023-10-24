#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "helpers/config.h"
#include "helpers/peer_connection.h"
#include "helpers/peer_data.h"

#define BUFFER_SIZE 65536

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::clog << "Usage: " << argv[0] << " <config_location>" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    auto config = CONFIG::get_config_from_file(argv[1]);
    std::clog << "[Config]" << std::endl << config << std::endl;
    auto peer_socket = PEER_CONNECTION::get_peer_udp(config);
    std::cout << std::endl << "[Updated Config]" << std::endl;
    std::cout << "> Self UDP Port     " << ntohs(config.self_udp_port_n) << std::endl;
    std::cout << "> Peer IP           " << inet_ntoa(*(in_addr*)&config.peer_ip_n) << std::endl;
    std::cout << "> Peer Port         " << ntohs(config.peer_port_n) << std::endl;
    
    ConnectionManager connection_manager(config, peer_socket);

    int tcp_raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (tcp_raw_socket < 0) 
    {
        perror("raw socket creation");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    if (setsockopt(tcp_raw_socket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) 
    {
        perror("setsockopt");
        exit(1);
    }

    int udp_raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (udp_raw_socket < 0) 
    {
        perror("raw socket creation");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(udp_raw_socket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) 
    {
        perror("setsockopt");
        exit(1);
    }

    connection_manager.handle_packets(tcp_raw_socket, udp_raw_socket);

    close(tcp_raw_socket);
    close(udp_raw_socket);
    close(peer_socket);
    return 0;
}