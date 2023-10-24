#include <string.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "helpers/config.h"
#include "helpers/peer_connection.h"
#include "helpers/peer_data.h"

#define BUFFER_SIZE 65536

Config get_input_args(const char* config_loc)
{
    std::ifstream inf{ config_loc };
    if (!inf)
    {
        std::cerr << "Uh oh, Sample.txt could not be opened for writing!" << std::endl;
        exit(EXIT_FAILURE);
    }

    Config config;
    memset(&config, 0, sizeof(config));

    auto ip_pton = [](const char* ip) -> uint32_t
    {
        int res = inet_addr(ip);
        if (res == -1)
            exit(EXIT_FAILURE);
        return res;
    };

    while (inf)
    {
        std::string input;
        inf >> input;

        if (input == "relay_ip")
        {
            inf >> input;
            config.relay_ip_n = ip_pton(input.c_str());
        }
        else if (input == "peer_ip")
        {
            inf >> input;
            config.peer_ip_n = ip_pton(input.c_str());
        }
        else if (input == "peer_port")
        {
            inf >> config.peer_port_n;
            config.peer_port_n = htons(config.peer_port_n);
        }
        else if (input == "relay_ip")
        {
            inf >> input;
            config.relay_ip_n = ip_pton(input.c_str());
        }
        else if (input == "relay_port")
        {
            inf >> config.relay_port_n;
            config.relay_port_n = htons(config.relay_port_n);
        }
        else if (input == "self_udp_port")
        {
            inf >> config.self_udp_port_n;
            config.self_udp_port_n = htons(config.self_udp_port_n);
        }
        else if (input == "self_server_port")
        {
            inf >> config.self_server_port_n;
            config.self_server_port_n = htons(config.self_server_port_n);
        }
        else if (input == "remote_mimic_port")
        {
            inf >> config.remote_mimic_port_n;
            config.remote_mimic_port_n = htons(config.remote_mimic_port_n);
        }
        else if (input == "identifier")
        {
            inf >> input;
            config.identifier = input;
        }
    }

    return config;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::clog << "Usage: " << argv[0] << " <config_location>" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    auto config = get_input_args(argv[1]);
    std::clog << "Config: " << std::endl;
    std::clog << config << std::endl;

    auto peer_socket = PEER::get_peer_udp(config);
    std::clog << "Succesfully connected with PEER!" << std::endl;
    
    return 0;
}