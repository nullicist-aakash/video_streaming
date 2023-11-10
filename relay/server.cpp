#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include "../network/data_types.h"
#include "../network/connection_manager.h"

#define MAX_PENDING 5
#define BUFFSIZE 32

using namespace Network;

struct Class1Compare
{
   bool operator() (const Socket& lhs, const Socket& rhs) const
   {
	   return lhs.ip.get_ip() < rhs.ip.get_ip() || (lhs.ip.get_ip() == rhs.ip.get_ip() && lhs.port.get_port() < rhs.port.get_port());
   }
};


class ConnectionManager
{
	std::map<Socket, std::string, Class1Compare> socket_to_identifier;
	std::map<std::string, std::vector<Socket>> identifier_to_sockets;

public:
	void register_connection(Socket addr, std::string& identifier)
	{
		remove_socket(addr);
		socket_to_identifier[addr] = identifier;
		identifier_to_sockets[identifier].push_back(addr);
	}

	void remove_socket(const Socket& sockfd)
	{
		if (socket_to_identifier.find(sockfd) == socket_to_identifier.end())
			return;

		std::string identifier = socket_to_identifier[sockfd];
		socket_to_identifier.erase(sockfd);
		auto& vec = identifier_to_sockets[identifier];

		for (int i = 0; i < vec.size(); ++i)
		{
			if (vec[i] != sockfd)
				continue;
			
			vec[i] = vec.back();
			vec.pop_back();
		}

		if (vec.empty())
			identifier_to_sockets.erase(identifier);
	}

	void remove_identifier(const std::string& identifier)
	{
		if (identifier_to_sockets.find(identifier) == identifier_to_sockets.end())
			return;

		auto& vec = identifier_to_sockets[identifier];
		for (int i = 0; i < vec.size(); ++i)
			socket_to_identifier.erase(vec[i]);
		
		identifier_to_sockets.erase(identifier);
	}

	const std::vector<Socket> get_peers(const std::string& identifier) const
	{
		if (get_connection_count(identifier) == 0)
			return {};

		return identifier_to_sockets.at(identifier);
	}

	int get_connection_count(const std::string& identifier) const
	{
		if (identifier_to_sockets.find(identifier) == identifier_to_sockets.end())
			return 0;
		return identifier_to_sockets.at(identifier).size();
	}
};

UDP handle_read(UDP udp, std::string& identifier, Socket peer, ConnectionManager& cm)
{
	identifier.erase(std::remove_if(identifier.begin(), identifier.end(), ::isspace), identifier.end());
	std::clog << peer << ": Received identifier `" << identifier << "`" << std::endl;

	cm.register_connection(peer, identifier);

	std::clog << peer << ": Number of peers under same identifier - " << cm.get_connection_count(identifier) << std::endl;
	if (cm.get_connection_count(identifier) != 2)
		return udp;
	
	auto& peers = cm.get_peers(identifier);
	for (int i = 0; i < peers.size(); ++i)
	{
		for (int j = 0; j < peers.size(); ++j)
		{
			if (i == j)
				continue;

			const auto& peer1 = peers[i];
			const auto& peer2 = peers[j];

			std::clog << peer1 << ": Will receive " << peer2 << std::endl;

			std::string bytes_to_send(sizeof(peer2), '\0');
			memcpy(bytes_to_send.data(), &peer2, sizeof(peer2));

			int times = 3;
			while (times--)
				udp.send(bytes_to_send, peer1);
		}
	}

	cm.remove_identifier(identifier);
	return udp;
}

int main(int argc, char** argv)
{
	if (argc != 2) 
	{
		std::clog << "Usage: " << argv[0] << " <server_port>" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << argv[1];
	uint16_t port;
	ss >> port;
	std::clog << "Will open the relay server (UDP) on port " << port << std::endl;
    
	UDP udp (PORT{port, ByteOrder::HOST});

	ConnectionManager cm;
	while (true)
	{
		auto [identifier, socket] = udp.receive();
		udp = handle_read(std::move(udp), identifier, socket, cm);
	}
}