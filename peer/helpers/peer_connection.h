#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace PEER
{
    struct PeerAddress
    {
        uint32_t self_ip;
        uint32_t peer_ip;
        uint16_t self_port;
        uint16_t peer_port;
    };

    enum class ConnectionState
    {
        STARTED,
        SENT_SELF,
        RECEIVED_PEER
    };

    PeerAddress get_pa_from_relay(int, const sockaddr_in&, const char*);
    void make_connection_with_peer(int, const char*, const PeerAddress&);
    int get_peer_udp(Config&);
}