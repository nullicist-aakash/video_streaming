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
    std::cout << "> Self UDP Port     " << config.self_udp_port << std::endl;
    std::cout << "> Peer IP           " << config.peer_info.ip << std::endl;
    std::cout << "> Peer Port         " << config.peer_info.port << std::endl;
    
    return 0;
    int raw_tcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_tcp_socket < 0)
    {
        perror("raw socket creation");
        exit(EXIT_FAILURE);
    }

    // PEER_CONNECTION::ConnectionManager cm(config, std::move(peer_socket), raw_tcp_socket);
    // packet_handler(cm);

    // close(raw_tcp_socket);
    return 0;
}