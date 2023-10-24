#pragma once
#include "config.h"

#include <netinet/ip.h>
#include <vector>
#include <map>
#include <set>

enum class PacketDirection
{
    UNKNOWN,
    LOCAL_SERVER_TO_REMOTE_CLIENT,
    LOCAL_CLIENT_TO_REMOTE_SERVER,
    REMOTE_SERVER_TO_LOCAL_CLIENT,
    REMOTE_CLIENT_TO_LOCAL_SERVER
};

class ConnectionManager
{
    const CONFIG::Config config;
    const std::vector<uint32_t> local_ips;
    // This should not be named as generated_to_remote due to convention
    std::map<int, int> rev_remote_to_generated;
    std::map<int, int> remote_to_generated;
    std::set<int> local_client_ports;

    bool is_local_ip(uint32_t) const;
    PacketDirection get_packet_direction(const iphdr*, int) const;

public:
    ConnectionManager(const CONFIG::Config& config);
};
