#include "data_types.h"
#include <string.h>
#include <cassert>
#include <stdexcept>
#include <ifaddrs.h>

uint32_t get_ip_n(std::string_view value)
{
    uint32_t ip;
    switch (inet_pton(AF_INET, value.data(), &ip))
    {
    case 0:
        throw std::runtime_error("IP address doesn't belong to IPv4 family");
    case -1:
        throw std::runtime_error("Invalid IP address");
    }

    return ip;
}

namespace Network
{
    IP::IP(std::string_view value) : ip4_n{ get_ip_n(value) }
    {

    }

    IP::IP(uint32_t value, ByteOrder order) : ip4_n
    {
        order == ByteOrder::NETWORK ? value : htonl(value)
    } { }


    std::vector<IP> IP::get_local_ips()
    {
        ifaddrs* ifAddrStruct = nullptr;
        getifaddrs(&ifAddrStruct);
        if (ifAddrStruct == nullptr)
            return {};

        bool found = false;

        std::vector<IP> ips;
        for (ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET)
                continue;

            auto ipn = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
            if (ipn == 0 || ipn == 0xFFFFFFFF)
                continue;

            ips.push_back(ipn);
        }

        freeifaddrs(ifAddrStruct);
        return ips;
    }

    uint32_t IP::get_ip(ByteOrder order) const
    {
        return order == ByteOrder::NETWORK ? ip4_n : ntohl(ip4_n);
    }

    IP::operator std::string() const
    {
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip4_n, str, INET_ADDRSTRLEN);
        return std::string(str);
    }

    std::ostream& operator<< (std::ostream& out, const IP& ip)
    {
        out << (std::string)ip;
        return out;
    }
    
    std::istream& operator>> (std::istream& in, IP& ip)
    {
        std::string str;
        in >> str;
        ip = IP(str);
        return in;
    }
}

namespace Network
{
    PORT::PORT(const uint16_t& value, ByteOrder order) : port_n
    {
        order == ByteOrder::NETWORK ? value : htons(value)
    } { }

    uint16_t PORT::get_port(ByteOrder order) const
    {
        return order == ByteOrder::NETWORK ? port_n : ntohs(port_n);
    }

    std::ostream& operator<< (std::ostream& out, const PORT& port)
    {
        out << port.get_port(ByteOrder::HOST);
        return out;
    }

    std::istream& operator>> (std::istream& in, PORT& port)
    {
        uint16_t prt;
        in >> prt;
        port = PORT(prt, ByteOrder::HOST);
        return in;
    }
}

namespace Network
{
    Socket::Socket(sockaddr_in& addr) : ip{ addr.sin_addr.s_addr }, port{ addr.sin_port } { }
    
    Socket::Socket(IP ip, PORT port) : ip{ ip }, port{ port } { }

    Socket::operator sockaddr_in() const
    {
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = ip.get_ip();
		addr.sin_port = port.get_port();
		return addr;
	}

    std::ostream& operator<< (std::ostream& out, const Socket& socket)
    {
        out << socket.ip << ":" << socket.port;
        return out;
    }

    std::ostream& operator<< (std::ostream& out, const SocketPair& sp)
    {
        out << sp.self << "::" << sp.remote;
        return out;
    }
}