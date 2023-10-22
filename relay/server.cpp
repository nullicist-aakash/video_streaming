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
#include <barrier>
#include <mutex>
#include <memory>
#include <cassert>
#include <algorithm>

#define MAX_PENDING 5
#define BUFFSIZE 32

const std::string get_printable_IP(const sockaddr_in& addr)
{
    // Using 'https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/i/inet_ntop.html'
    char printable_address[INET_ADDRSTRLEN];
    
	if (inet_ntop(addr.sin_family, &addr.sin_addr, printable_address, sizeof(printable_address)) != nullptr)
        return std::string(printable_address) + ":" + std::to_string(ntohs(addr.sin_port));
    
	return "ERROR:" + std::to_string(ntohs(addr.sin_port));
}

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
	std::map<std::string, std::barrier<>*> identifier_to_barrier;

	const static int max_connections = 2;
	std::mutex _mtx;
public:
	bool is_connection_full(std::string& identifier)
	{
		if (identifier_to_fds.find(identifier) == identifier_to_fds.end())
			return false;

		return identifier_to_fds.at(identifier).size() == max_connections;
	}

	bool register_connection(int fd, sockaddr_in addr, std::string& identifier)
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (is_connection_full(identifier))
			return false;

		assert(connections.find(fd) == connections.end());

		connections[fd] = addr;
		identifier_to_fds[identifier].push_back(fd);
		fd_to_identifier[fd] = identifier;

		if (identifier_to_barrier[identifier] == nullptr)
			identifier_to_barrier[identifier] = new std::barrier(max_connections);
		return true;
	}

	std::vector<std::pair<int, sockaddr_in>> get_peers(int fd)
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (connections.find(fd) == connections.end())
			return {};

		const auto& identifier = fd_to_identifier.at(fd);
		std::vector<int> fds = identifier_to_fds.at(identifier);
		std::vector<std::pair<int, sockaddr_in>> peers;
		for (int fd : fds)
			peers.push_back({fd, connections.at(fd)});
		return peers;
	}

	bool remove_fd(int fd)
	{
		std::lock_guard<std::mutex> lock(_mtx);
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
			delete identifier_to_barrier[identifier];
			identifier_to_barrier.erase(identifier);
		}

		connections.erase(fd);
		fd_to_identifier.erase(fd);
		return true;
	}

	bool remove_by_identifier(const std::string& identifier)
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (identifier_to_fds.find(identifier) == identifier_to_fds.end())
			return false;

		auto &vec = identifier_to_fds[identifier];
		for (int fd : vec)
		{
			connections.erase(fd);
			fd_to_identifier.erase(fd);
		}

		identifier_to_fds.erase(identifier);
		delete identifier_to_barrier[identifier];
		identifier_to_barrier.erase(identifier);
		return true;
	}

	void wait_on_barrier(int fd)
	{
		{
			std::lock_guard<std::mutex> lock(_mtx);
			if (fd_to_identifier.find(fd) == fd_to_identifier.end())
				return;
		}

		const auto& identifier = fd_to_identifier.at(fd);
		
		identifier_to_barrier.at(identifier)->arrive_and_wait();
	}
};

void handle_connection(int clifd, sockaddr_in cliaddr, ConnectionManager& cm)
{
	auto prefix = get_printable_IP(cliaddr) + ": ";
	
	char BUFF[BUFFSIZE];
	int n = read(clifd, BUFF, BUFFSIZE);
	if (n < 0) 
	{
		perror("read");
		std::clog << prefix << "Connection terminated unexpectedly!" << std::endl;
		close(clifd);
		return;
	}

	if (n == 0)
	{
		std::clog << prefix << "Client closed the connection without sending identifier!" << std::endl;
		close(clifd);
		return;
	}

	std::string identifier(BUFF, n);
	identifier.erase(std::remove_if(identifier.begin(), identifier.end(), ::isspace), identifier.end());
	std::clog << prefix << "Client sent the identifier: `" << identifier << "`" << std::endl;

	if (!cm.register_connection(clifd, cliaddr, identifier))
	{
		std::clog << prefix << "2 clients are already waiting on this identifier. Closing the connection." << std::endl;
		close(clifd);
		return;
	}

	cm.wait_on_barrier(clifd);
	auto peers = cm.get_peers(clifd);

	for (auto &[fd, addr]: peers)
	{
		if (fd == clifd)
			continue;
		
		std::clog << prefix << "Sending PEER information to the client - " << get_printable_IP(addr) << std::endl;
		
		if ((n = write(clifd, &addr, sizeof(addr))) < 0)
		{
			std::clog << prefix << "Client closed the connection without receiving peer!" << std::endl;
			perror("write");
		}
	}
	
	cm.wait_on_barrier(clifd);
	
	std::clog << prefix << "Closing the connection." << std::endl;
	close(clifd);

	if (cm.remove_by_identifier(identifier))
		std::clog << prefix << "Cleared the identifier `" << identifier << "`" << std::endl;
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

		std::thread([client_fd, cliaddr, &cm]() {
			std::clog << get_printable_IP(cliaddr) << ": Accepted a new connection." << std::endl;
			handle_connection(client_fd, cliaddr, cm);
		}).detach();
	};
}