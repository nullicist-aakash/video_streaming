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
#include <thread>
#include <mutex>
#include <barrier>
#include <memory>

#define MAX_PENDING 5
#define BUFFSIZE 32

int passive_tcp_socket(int self_port) 
{
	// Register TCP Socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket error");
		exit(-1);
	}
	
	// Bind to local port
	struct sockaddr_in selfAddr;
	memset(&selfAddr, 0, sizeof(selfAddr));
	selfAddr.sin_family = AF_INET;
	selfAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Bind to any available local interface
	selfAddr.sin_port = htons(self_port);  // Specify the port you want to bind to

	if (bind(listenfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) == -1) {
		perror("bind_listen");
		close(listenfd);
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(listenfd, MAX_PENDING) == -1) {
		perror("listen");
		close(listenfd);
		exit(EXIT_FAILURE);
	}

	return listenfd;
}

class ConnectionManager
{
	std::map<int, sockaddr_in> connections;  // fd -> sockaddr_in
	std::map<int, std::string> fd_to_identifier;
	std::map<std::string, std::vector<int>> identifier_to_fds;
	std::map<std::string, std::unique_ptr<std::barrier<std::mutex>>> identifier_to_barrier;

	const static int max_connections = 2;
public:
	bool is_connection_full(std::string& identifier) const
	{
		if (identifier_to_fds.find(identifier) == identifier_to_fds.end())
			return false;

		return identifier_to_fds.at(identifier).size() == max_connections;
	}

	bool register_connection(int fd, sockaddr_in addr, std::string& identifier)
	{
		if (is_connection_full(identifier))
			return false;

		if (connections.find(fd) != connections.end())
			return false;

		connections[fd] = addr;
		identifier_to_fds[identifier].push_back(fd);
		fd_to_identifier[fd] = identifier;
		identifier_to_barrier[identifier] = std::make_unique<std::barrier<std::mutex>>(new std::barrier<std::mutex>(max_connections));
		return true;
	}

	std::vector<sockaddr_in> get_peers(int fd)
	{
		if (connections.find(fd) == connections.end())
			return {};

		const auto& identifier = fd_to_identifier.at(fd);
		std::vector<int> fds = identifier_to_fds.at(identifier);
		std::vector<sockaddr_in> peers;
		for (int fd : fds)
			peers.push_back(connections.at(fd));
		return peers;
	}

	bool remove_fd(int fd)
	{
		if (connections.find(fd) == connections.end())
			return false;
		
		const auto& identifier = fd_to_identifier.at(fd);
		auto &vec = identifier_to_fds[identifier];
		for (int i = 0; i < vec.size(); ++i)
		{
			if (vec[i] != fd)
				continue;
				
			vec[i] = vec.back();
			vec.pop_back();
			break;
		}

		if (vec.size() == 0)
		{
			identifier_to_fds.erase(identifier);
			identifier_to_barrier.erase(identifier);
		}

		connections.erase(fd);
		fd_to_identifier.erase(fd);
		return true;
	}

	bool remove_by_identifier(const std::string& identifier)
	{
		if (identifier_to_fds.find(identifier) == identifier_to_fds.end())
			return false;

		auto &vec = identifier_to_fds[identifier];
		for (int fd : vec)
		{
			connections.erase(fd);
			fd_to_identifier.erase(fd);
		}
		
		identifier_to_fds.erase(identifier);
		identifier_to_barrier.erase(identifier);
		return true;
	}

	void wait_on_barrier(int fd)
	{
		if (fd_to_identifier.find(fd) == fd_to_identifier.end())
			return;

		const auto& identifier = fd_to_identifier.at(fd);
		identifier_to_barrier.at(identifier)->arrive_and_wait();
	}
};

void handle_connection(int clifd, sockaddr_in& cliaddr, ConnectionManager& cm)
{

}

int main(int argc, char** argv)
{
	std::map<int, int> mp;
	std::cout << mp.at(1) << std::endl;
	return 0;
	if (argc != 2) 
	{
		std::clog << "Usage: " << argv[0] << " <server_port>" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << argv[1];
	int port;
	ss >> port;
	std::clog << "Will open the realy server on port " << port << std::endl;

    int passive_fd = passive_tcp_socket(port);
	ConnectionManager cm;

	while (true)
	{
		sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int client_fd = accept(passive_fd, (sockaddr*)&cliaddr, &clilen);

		if (client_fd < 0) 
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		std::clog << "Accepted a new connection from " << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << std::endl;
		handle_connection(client_fd, cliaddr, cm);
	}
}