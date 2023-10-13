#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

using namespace std;

#define MAX_PENDING 5
#define BUFFSIZE 32

int active_tcp_sokcet(char* server_ip, int server_port, int self_port) 
{
	// Register TCP Socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket error");
		exit(-1);
	}
	
	// Open socket with reuse option
	int optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// Bind to local port
	struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Bind to any available local interface
    clientAddr.sin_port = htons(self_port);  // Specify the port you want to bind to

    if (bind(sockfd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1) {
        perror("bind_send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

	// Fill the server details
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0)
	{
		perror("inet_pton error");
		exit(1);
	}

	// perform three way handshake
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect error");
		exit(1);
	}

	return sockfd;
}

int passive_tcp_socket(int self_port) 
{
	// Register TCP Socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket error");
		exit(-1);
	}
	
	// Open socket with reuse option
	int optval = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
		perror("setsockopt");
		close(listenfd);
		exit(EXIT_FAILURE);
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

void handle_listener(int self_port)
{
	char buff[BUFFSIZE];
	memset(buff, 0, BUFFSIZE);

	int listen_fd = passive_tcp_socket(self_port);

	// Wait for connection from PEER
	int connfd = accept(listen_fd, nullptr, nullptr);
	int n = read(connfd, buff, BUFFSIZE);
	buff[n] = '\0';
	cout << "Received: " << buff << endl;
	close(connfd);
}

int main(int argc, char** argv)
{
	if (argc != 5)
	{
		cout << "usage: client.o <SERVER_IP> <SERVER_PORT> <SELF_PORT> <KEY>" << endl;
		exit(1);
	}

	char* server_ip = argv[1];
	int server_port = atoi(argv[2]);
	int self_port = atoi(argv[3]);
	char* key = argv[4];

	// Step 1: Send a secret to server
	int relay_fd = active_tcp_sokcet(server_ip, server_port, self_port);
	write(relay_fd, key, strlen(key));

	// Step 2: Create a parallel connection for listening
	thread t1(handle_listener, self_port);
	this_thread::sleep_for(chrono::milliseconds(1000));

	// Step 3: Wait for remote IP:PORT from server
	char buff[BUFFSIZE];
	int n = read(relay_fd, buff, BUFFSIZE);
	buff[n] = '\0';
	cout << "Will make connection to PEER on: " << buff << endl;

	// Extract IP and PORT from the received string
	string str(buff);
	size_t found = str.find(':');
	assert(found != std::string::npos);
	string remote_ip = str.substr(0, found);
	int remote_port = atoi(str.substr(found + 1).c_str());

	// Step 4: Send data to PEER
	int peer_fd = active_tcp_sokcet((char*)remote_ip.c_str(), remote_port, self_port);
	const char* peer_msg = "Hello Peer";
	write(peer_fd, peer_msg, strlen(peer_msg));

	close(relay_fd);
	close(peer_fd);

	t1.join();
	return 0;
}
