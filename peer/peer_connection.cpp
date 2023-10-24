#include "peer_connection.h"
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <thread>

namespace PEER
{
    sockaddr_in get_sockaddr(const char* ip, const int port)
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);

        if (addr.sin_addr.s_addr == -1)
            exit(EXIT_FAILURE);
        return addr;
    }

    PeerAddress get_pa_from_relay(int relay_socket, const sockaddr_in& relay_addr, const char* identifier)
    {
        auto identifier_len = strlen(identifier);
        ConnectionState state = ConnectionState::STARTED;

        auto thread = std::thread([&]() 
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                sendto(relay_socket, identifier, identifier_len, 0, (sockaddr*)&relay_addr, sizeof(relay_addr));
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });

        PeerAddress pa;
        while (true)
        {
            int n = recvfrom(relay_socket, &pa, sizeof(pa), 0, NULL, NULL);
            if (n < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            if (n == sizeof(pa) && pa.peer_ip != 0 && pa.peer_port != 0)
                break;
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
        return pa;
    }

    void make_connection_with_peer(int relay_socket, const char* identifier, const PeerAddress& pa)
    {
        sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(peer_addr));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = pa.peer_port;
        peer_addr.sin_addr.s_addr = pa.peer_ip;

        ConnectionState state = ConnectionState::STARTED;
        auto identifier_len = strlen(identifier);

        // send messages in a loop to peer, untill we receive something from other side
        auto thread = std::thread([&]() 
        {
            while (state != ConnectionState::RECEIVED_PEER)
            {
                state = ConnectionState::SENT_SELF;
                sendto(relay_socket, identifier, identifier_len, 0, (sockaddr*)&peer_addr, sizeof(peer_addr));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        
        while (true)
        {
            char BUFF[strlen(identifier) + 1];
            memset(BUFF, 0, sizeof(BUFF));

            if (recvfrom(relay_socket, BUFF, sizeof(BUFF), 0, NULL, NULL) < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            if (strcmp(BUFF, identifier) == 0 && pa.peer_ip == peer_addr.sin_addr.s_addr && pa.peer_port == peer_addr.sin_port)
            {
                std::cout << "Received from peer: " << BUFF << std::endl;
                break;
            }
        }

        state = ConnectionState::RECEIVED_PEER;
        thread.join();
    }

    int get_peer_udp(const char* relay_ip, const int relay_port, const char* identifier)
    {
        auto relay_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (relay_socket < 0)
        {
            perror("udp socket");
            exit(EXIT_FAILURE);
        }

        auto relay_addr = get_sockaddr(relay_ip, relay_port);
        auto pa = get_pa_from_relay(relay_socket, relay_addr, identifier);
        make_connection_with_peer(relay_socket, identifier, pa);

        return relay_socket;
    }
}