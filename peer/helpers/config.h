#pragma once
#include <cstdint>
#include <string_view>
#include <ostream>
#include "../../network/data_types.h"

using namespace Network;

namespace CONFIG
{
    struct Config
    {
        Socket relay_info{};
        Socket peer_info{};
        PORT self_udp_port{};
        PORT self_server_port{};
        PORT self_mimic_port{};
        std::string identifier{};

        friend std::ostream& operator<<(std::ostream& os, const Config& config);
    };

    Config get_config_from_file(std::string_view config_loc);
}