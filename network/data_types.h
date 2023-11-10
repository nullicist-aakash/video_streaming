#pragma once
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <ostream>
#include <istream>

namespace Network
{
    enum class ByteOrder
    {
        NETWORK,
        HOST
    };

    class IP
    {
        uint32_t ip4_n{ INADDR_ANY };
    public:
        IP(uint32_t value = INADDR_ANY, ByteOrder order = ByteOrder::NETWORK);
        IP(std::string_view value);

        static std::vector<IP> get_local_ips();

        uint32_t get_ip(ByteOrder order = ByteOrder::NETWORK) const;
        operator std::string() const;

        bool operator==(const IP& other) const
        {
            return ip4_n == other.ip4_n;
        }

        bool operator!=(const IP& other) const
        {
            return ip4_n != other.ip4_n;
        }

        friend std::ostream& operator<< (std::ostream&, const IP&);
        friend std::istream& operator>> (std::istream&, IP&);
    };

    class PORT
    {
        uint16_t port_n{ 0 };
    public:
        PORT(const uint16_t& value = 0, ByteOrder order = ByteOrder::NETWORK);
        uint16_t get_port(ByteOrder order = ByteOrder::NETWORK) const;

        bool operator==(const PORT& other) const
        {
            return port_n == other.port_n;
        }

        bool operator!=(const PORT& other) const
        {
            return port_n != other.port_n;
        }

        friend std::ostream& operator<< (std::ostream&, const PORT&);
        friend std::istream& operator>> (std::istream&, PORT&);
    };
    
    class Socket
    {
    public:
        IP ip{};
        PORT port{};

        Socket(sockaddr_in& addr);
        Socket(IP ip = {}, PORT port = {});

        bool operator==(const Socket& other) const
        {
            return (ip == other.ip) && (port == other.port);
        }

        bool operator!=(const Socket& other) const
        {
            return (ip != other.ip) || (port != other.port);
        }

        operator sockaddr_in() const;
        friend std::ostream& operator<< (std::ostream&, const Socket&);
    };

    struct SocketPair
    {
        Socket self{};
        Socket remote{};

        bool operator==(const SocketPair& other) const
        {
            return (self == other.self) && (remote == other.remote);
        }

        bool operator!=(const SocketPair& other) const
        {
            return (self != other.self) || (remote != other.remote);
        }
        friend std::ostream& operator<< (std::ostream&, const SocketPair&);
    };
}