#include "connection.h"
#include <iostream>

bool SocketCompare::operator() (const sockaddr_in& lhs, const sockaddr_in& rhs) const
{
    if (lhs.sin_addr.s_addr < rhs.sin_addr.s_addr)
        return true;

    if (lhs.sin_addr.s_addr > rhs.sin_addr.s_addr)
        return false;

    return lhs.sin_port < rhs.sin_port;
}

const std::string get_printable_IP(const in_addr& addr)
{
    // Using 'https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/i/inet_ntop.html'
    char printable_address[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, printable_address, sizeof(printable_address)) != nullptr)
        return printable_address;
    else
        return "ERROR";
}

Connection::Connection() { }
Connection::~Connection() { }

void Connection::add_entry(sockaddr_in& addr, const std::string& identifier)
{
    std::clog << "Adding entry: " << identifier << " " << get_printable_IP(addr.sin_addr) << ":" << addr.sin_port << std::endl;
    
    // Repalce the old entry if it exists
    if (socket_to_identifier.find(addr) != socket_to_identifier.end())
        remove_by_addr(addr);
    
    // Add new entry
    identifier_to_sockets[identifier].push_back(addr);
    socket_to_identifier[addr] = identifier;
}

void Connection::remove_by_addr(sockaddr_in& addr)
{
    std::clog << "Removing entry by address: " << get_printable_IP(addr.sin_addr) << ":" << addr.sin_port << std::endl;
    
    if (socket_to_identifier.find(addr) == socket_to_identifier.end())
        return;
        
    // Remove from identifier_to_sockets
    auto identifier = socket_to_identifier[addr];
    auto& sockets = identifier_to_sockets[identifier];
    for (auto it = sockets.begin(); it != sockets.end(); ++it)
    {
        if (it->sin_addr.s_addr == addr.sin_addr.s_addr && it->sin_port == addr.sin_port)
        {
            sockets.erase(it);
            break;
        }
    }

    if (sockets.size() == 0)
        identifier_to_sockets.erase(identifier);

    // Remove from socket_to_identifier
    socket_to_identifier.erase(addr);
}

void Connection::remove_by_identifier(const std::string &identifier)
{
    std::clog << "Removing entry by Identifier: " << identifier << std::endl;
    
    if (identifier_to_sockets.find(identifier) == identifier_to_sockets.end())
        return;

    // Remove from socket_to_identifier
    auto& sockets = identifier_to_sockets[identifier];
    for (auto& addr : sockets)
        socket_to_identifier.erase(addr);

    // Remove from identifier_to_sockets
    identifier_to_sockets.erase(identifier);
}

const std::vector<sockaddr_in> Connection::get_sockets_by_identifier(const std::string& identifier) const
{
    std::clog << "Retrieving sockets by identifier: " << identifier << std::endl;
 
    if (identifier_to_sockets.find(identifier) == identifier_to_sockets.end())
        return {};

    return identifier_to_sockets.at(identifier);
}

int Connection::get_socket_count_by_identifier(const std::string& identifier) const
{
    std::clog << "Retrieving socket count by identifier: " << identifier << std::endl;
    
    if (identifier_to_sockets.find(identifier) == identifier_to_sockets.end())
        return 0;

    return identifier_to_sockets.at(identifier).size();
}