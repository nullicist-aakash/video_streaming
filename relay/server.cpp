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

#define MAX_PENDING 5
#define BUFFSIZE 32

int comp(const sockaddr_in& a, const sockaddr_in& b)
{
	if (a.sin_addr.s_addr < b.sin_addr.s_addr)
		return -1;
	if (a.sin_addr.s_addr > b.sin_addr.s_addr)
		return 1;
	if (a.sin_port < b.sin_port)
		return -1;
	if (a.sin_port > b.sin_port)
		return 1;
	
	return 0;
}

struct AddrComparator
{
   bool operator() (const sockaddr_in& lhs, const sockaddr_in& rhs) const
   {
       return comp(lhs, rhs);
   }
};

struct PeerAddress
{
	uint32_t self_ip;
	uint32_t peer_ip;
	uint16_t self_port;
	uint16_t peer_port;
};

const std::string get_printable_IP(const sockaddr_in& addr)
{
    // Using 'https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/i/inet_ntop.html'
    char printable_address[INET_ADDRSTRLEN];
    
	if (inet_ntop(addr.sin_family, &addr.sin_addr, printable_address, sizeof(printable_address)) != nullptr)
        return std::string(printable_address) + ":" + std::to_string(ntohs(addr.sin_port));

	return "ERROR:" + std::to_string(ntohs(addr.sin_port));
}

class ConnectionManager
{
	std::map<sockaddr_in, std::string, AddrComparator> socket_to_identifier;
	std::map<std::string, std::vector<sockaddr_in>> identifier_to_sockets;

public:
	void register_connection(sockaddr_in addr, std::string& identifier)
	{
		remove_socket(addr);
		socket_to_identifier[addr] = identifier;
		identifier_to_sockets[identifier].push_back(addr);
	}

	void remove_socket(const sockaddr_in& sockfd)
	{
		if (socket_to_identifier.find(sockfd) == socket_to_identifier.end())
			return;

		std::string identifier = socket_to_identifier[sockfd];
		socket_to_identifier.erase(sockfd);
		auto& vec = identifier_to_sockets[identifier];

		for (int i = 0; i < vec.size(); ++i)
		{
			if (comp(vec[i], sockfd) != 0)
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

	const std::vector<sockaddr_in> get_peers(const std::string& identifier) const
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

void handle_read(int sockfd, sockaddr_in& sender, std::string& identifier, ConnectionManager& cm)
{
	auto prefix = get_printable_IP(sender) + ": ";
	identifier.erase(std::remove_if(identifier.begin(), identifier.end(), ::isspace), identifier.end());
	std::clog << prefix << "Received identifier `" << identifier << "`" << std::endl;

	cm.register_connection(sender, identifier);

	std::clog << prefix << "Number of peers under same identifier - " << cm.get_connection_count(identifier) << std::endl;
	if (cm.get_connection_count(identifier) != 2)
		return;
	
	auto& peers = cm.get_peers(identifier);
	for (int i = 0; i < peers.size(); ++i)
	{
		for (int j = 0; j < peers.size(); ++j)
		{
			if (i == j)
				continue;

			const sockaddr_in& peer1 = peers[i];
			const sockaddr_in& peer2 = peers[j];
			auto msg = get_printable_IP(peer2);

			std::clog << get_printable_IP(peer1) << ": Will receive " << msg << std::endl;

			PeerAddress pa;
			pa.self_ip = peer1.sin_addr.s_addr;
			pa.self_port = peer1.sin_port;
			pa.peer_ip = peer2.sin_addr.s_addr;
			pa.peer_port = peer2.sin_port;

			int times = 3;
			while (times--)
				sendto(sockfd, (char*)&pa, sizeof(pa), 0, (struct sockaddr*)&peer1, sizeof(peer1));
		}
	}

	cm.remove_identifier(identifier);
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
	int port;
	ss >> port;
	std::clog << "Will open the relay server (UDP) on port " << port << std::endl;
    
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
	{
		perror("socket error");
		exit(-1);
	}
	
	// Bind to local port
	struct sockaddr_in selfAddr;
	memset(&selfAddr, 0, sizeof(selfAddr));
	selfAddr.sin_family = AF_INET;
	selfAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Bind to any available local interface
	selfAddr.sin_port = htons(port);  // Specify the port you want to bind to

	if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) == -1) 
	{
		perror("bind_listen");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	ConnectionManager cm;
	while (true)
	{
		char BUFF[BUFFSIZE];
		sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int n = recvfrom(sockfd, BUFF, BUFFSIZE, 0, (struct sockaddr*)&cliaddr, &clilen);
		if (n < 0) 
		{
			perror("recvfrom");
			continue;
		}

		std::string identifier(BUFF, n);
		handle_read(sockfd, cliaddr, identifier, cm);
	}
}