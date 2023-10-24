#include "config.h"
#include <arpa/inet.h>
#include <string.h>

#include <fstream>
#include <iostream>

namespace CONFIG
{
    std::ostream& operator<<(std::ostream& os, const Config& config)
    {
        os << "> Relay IP          " << inet_ntoa(*(in_addr*)&config.relay_ip_n) << std::endl;
        os << "> Relay Port        " << ntohs(config.relay_port_n) << std::endl;
        os << std::endl;
        os << "> Self UDP Port     " << ntohs(config.self_udp_port_n) << std::endl;
        os << "> Self Server Port  " << ntohs(config.self_server_port_n) << std::endl;
        os << "> Self Mimic Port   " << ntohs(config.self_mimic_port_n) << std::endl;
        os << std::endl;
        os << "> Identifier        " << config.identifier << std::endl;
        os << std::endl;
        os << "> Peer IP           " << inet_ntoa(*(in_addr*)&config.peer_ip_n) << std::endl;
        os << "> Peer Port         " << ntohs(config.peer_port_n) << std::endl;
        return os;
    }

    Config get_config_from_file(const char* config_loc)
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
            else if (input == "self_mimic_port")
            {
                inf >> config.self_mimic_port_n;
                config.self_mimic_port_n = htons(config.self_mimic_port_n);
            }
            else if (input == "identifier")
            {
                inf >> input;
                config.identifier = input;
            }
        }

        return config;
    }
}