#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "helpers/config.h"
#include "helpers/peer_connection.h"
#include "helpers/packet_builder.h"

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
    
    int raw_tcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_tcp_socket < 0)
    {
        perror("raw socket creation");
        exit(EXIT_FAILURE);
    }

    PEER_CONNECTION::ConnectionManager cm(config, peer_socket, raw_tcp_socket);
    packet_handler(cm);

    close(raw_tcp_socket);
    close(peer_socket);
    return 0;
}