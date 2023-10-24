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

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::clog << "Usage: " << argv[0] << " <config_location>" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    auto config = get_config_from_file(argv[1]);
    std::clog << "Config: " << std::endl;
    std::clog << config << std::endl;

    auto peer_socket = PEER::get_peer_udp(config);
    std::clog << "Succesfully connected with PEER!" << std::endl;
    
    return 0;
}