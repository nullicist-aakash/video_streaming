#pragma once
#include "peer_connection.h"
using namespace PEER_CONNECTION;

enum class PacketDirection
{
    UNKNOWN,
    LOCAL_SERVER_TO_REMOTE_CLIENT,
    LOCAL_CLIENT_TO_REMOTE_SERVER,
    REMOTE_SERVER_TO_LOCAL_CLIENT,
    REMOTE_CLIENT_TO_LOCAL_SERVER
};

void packet_handler(ConnectionManager&);