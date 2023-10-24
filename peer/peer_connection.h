#include <netinet/in.h>

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

    sockaddr_in get_sockaddr(const char*, const int);
    PeerAddress get_pa_from_relay(int, const sockaddr_in&, const char*);
    void make_connection_with_peer(int, const char*, const PeerAddress&);
    int get_peer_udp(const char* relay_ip, const int relay_port, const char* identifier);
}