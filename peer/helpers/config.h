#pragma once
#include <cstdint>
#include <string>
#include <ostream>

namespace CONFIG
{
    struct Config
    {
        uint32_t relay_ip_n;
        uint16_t relay_port_n;
        uint32_t peer_ip_n;
        uint16_t peer_port_n;
        uint16_t self_udp_port_n;
        uint16_t self_server_port_n;
        uint16_t self_mimic_port_n;
        std::string identifier;

        friend std::ostream& operator<<(std::ostream& os, const Config& config);
    };

    Config get_config_from_file(const char* config_loc);
}