#include "config.h"
#include <string.h>

#include <fstream>
#include <iostream>
#include "../network/dns.h"

namespace CONFIG
{
    std::ostream& operator<<(std::ostream& os, const Config& config)
    {
        os << "> Relay             " << config.relay_info << std::endl;
        os << "> Peer              " << config.peer_info << std::endl;
        os << std::endl;
        os << "> Self UDP Port     " << config.self_udp_port << std::endl;
        os << "> Self Server Port  " << config.self_server_port << std::endl;
        os << "> Self Mimic Port   " << config.self_mimic_port << std::endl;
        os << std::endl;
        os << "> Identifier        " << config.identifier << std::endl;
        return os;
    }

    Config get_config_from_file(std::string_view config_loc)
    {
        std::ifstream inf { config_loc.data() };

        if (!inf)
            throw std::runtime_error("Uh oh, Sample.txt could not be opened for writing!");

        Config config {};

        while (inf)
        {
            std::string input;
            inf >> input;

            if (input == "relay_ip")
            {
                std::string input;
                inf >> input;
                config.relay_info.ip = get_dns_response(input).ips[0];
            }
            else if (input == "relay_port")
                inf >> config.relay_info.port;
            else if (input == "peer_ip")
            {   
                std::string input;
                inf >> input;
                config.peer_info.ip = get_dns_response(input).ips[0];
            }
            else if (input == "peer_port")
                inf >> config.peer_info.port;
            else if (input == "self_udp_port")
                inf >> config.self_udp_port;
            else if (input == "self_server_port")
                inf >> config.self_server_port;
            else if (input == "self_mimic_port")
                inf >> config.self_mimic_port;
            else if (input == "identifier")
                inf >> config.identifier;
        }

        return config;
    }
}