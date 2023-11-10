#pragma once
#include "data_types.h"
#include <utility>

namespace Network
{
	static const int READ_MAX_SIZE = 65535;
	class TCP;

	class TCPServer
	{
		static const int LISTEN_LEN = 5;
		int listenfd{ -1 };
		const Socket self_socket;
	public:
		TCPServer(const Socket&);

		TCPServer(TCPServer&) = delete;
		TCPServer& operator=(TCPServer&) = delete;
		TCPServer(TCPServer&&);
		TCPServer& operator=(TCPServer&&);

		const Socket& get_socket() const;
		TCP accept() const;
		void close();

		~TCPServer();
	};

	class TCP
	{
		int sockfd{ -1 };
		SocketPair socket_pair;

		friend class TCPServer;
		TCP(int, const SocketPair&);

	public:
		TCP(const SocketPair&);

		TCP(TCP&) = delete;
		TCP& operator=(TCP&) = delete;
		TCP(TCP&&);
		TCP& operator=(TCP&&);

		int send(const std::string&) const;
		std::string receive(size_t n = 0) const;
		const SocketPair& get_socket_pair() const;
		void close();

		~TCP();
	};

	class UDP
	{
		int sockfd{ -1 };

	public:
		UDP();
		UDP(const Socket& self_socket);
		UDP(const PORT& self_port);

		UDP(UDP&) = delete;
		UDP& operator=(UDP&) = delete;
		UDP(UDP&&);
		UDP& operator=(UDP&&);

		void send(const std::string&, const Socket&) const;
		std::pair<std::string, Socket> receive() const;
		const PORT get_self_port() const;

		void close();

		~UDP();
	};
}	