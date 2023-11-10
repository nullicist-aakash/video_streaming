#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <thread>

#include "helpers/config.h"
#include "helpers/peer_connection.h"
#include "helpers/packet_builder.h"

enum class ConnectionState
{
    STARTED,
    SENT_SELF,
    RECEIVED_PEER
};

Socket get_peer_from_relay(const UDP& udp, CONFIG::Config& config)
{
    ConnectionState state = ConnectionState::STARTED;

    auto thread = std::thread([&]()
    {
        while (state != ConnectionState::RECEIVED_PEER)
        {
            state = ConnectionState::SENT_SELF;
            udp.send(config.identifier, config.relay_info);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    Socket pa;
    while (true)
    {
        auto [str, sock] = udp.receive();

        if ((str.size() != sizeof(pa)) || sock != config.relay_info)
            continue;

        memcpy(&pa, str.c_str(), sizeof(pa));
        break;
    }

    state = ConnectionState::RECEIVED_PEER;
    thread.join();
    return pa;
}

void make_connection_with_peer(const UDP& udp, CONFIG::Config& config)
{
    ConnectionState state = ConnectionState::STARTED;
    
    // send messages in a loop to peer, untill we receive something from other side
    auto thread = std::thread([&]() 
    {
        while (state != ConnectionState::RECEIVED_PEER)
        {
            state = ConnectionState::SENT_SELF;
            udp.send(config.identifier, config.peer_info);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    while (true)
    {
        auto [str, sock] = udp.receive();

        if (str != config.identifier || config.peer_info != sock)
            continue;
            
        std::cout << "Received from peer: " << str << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        break;
    }

    state = ConnectionState::RECEIVED_PEER;
    thread.join();
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::clog << "Usage: " << argv[0] << " <config_location>" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    auto config = CONFIG::get_config_from_file(argv[1]);
    std::clog << "[Config]" << std::endl << config << std::endl;

    UDP udp(config.self_udp_port);
    
    if (config.peer_info.ip == IP{} || config.peer_info.port == PORT{} || config.self_udp_port == PORT{})
    {
        std::cout << "Using relay server to get peer information" << std::endl;
        config.peer_info = get_peer_from_relay(udp, config);
        config.self_udp_port = udp.get_self_port();
    }

    make_connection_with_peer(udp, config);
    
    std::cout << std::endl << "[Updated Config]" << std::endl;
    std::clog << config << std::endl;
    
    int raw_tcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_tcp_socket < 0)
    {
        perror("raw socket creation");
        exit(EXIT_FAILURE);
    }

    ConnectionManager cm(config, std::move(udp), raw_tcp_socket);
    packet_handler(cm);

    close(raw_tcp_socket);
    return 0;
}