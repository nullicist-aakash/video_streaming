#include "connection_manager.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#define HEX( x, len ) std::setw(2 * len) << std::setfill('0') << std::hex << std::uppercase << (((1ll << (8 * len)) - 1) & (unsigned int)( x )) << std::dec

namespace Network
{
	void hex_view(const char* str, const size_t len)
	{
		for (int i = 0; i < ((int)len >> 4) + (len % 16 ? 1 : 0); ++i)
		{
			int start = i * 16;
			int end = std::min(i * 16 + 15, (int)len - 1);
			std::clog << "0x" << HEX(start, 4) << " |  ";
			for (int j = start; j <= end; ++j)
				std::clog << HEX(str[j], 1) << (j % 8 == 7 ? "  " : " ");

			for (int j = end; j < i * 16 + 15; ++j)
				std::clog << (j % 8 == 7 ? "    " : "   ");
			std::clog << "| ";
			for (int j = start; j <= end; ++j)
				std::clog << (std::isprint(str[j]) ? str[j] : '.');
			std::clog << '\n';
		}
	}

	TCPServer::TCPServer(const Socket& self_socket) : self_socket(self_socket)
	{
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		if (listenfd < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("socket: " + std::string(err));
		}

		struct sockaddr_in serv_addr {};
		explicit_bzero((char*)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = self_socket.ip.get_ip();
		serv_addr.sin_port = self_socket.port.get_port();

		if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("bind: " + std::string(err));
		}

		if (listen(listenfd, LISTEN_LEN) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("listen: " + std::string(err));
		}
	}

	TCPServer::TCPServer(TCPServer&& other)
	{
		this->listenfd = other.listenfd;
		other.listenfd = -1;
	}

	TCPServer& TCPServer::operator=(TCPServer&& other)
	{
		if (&other == this)
			return *this;

		this->listenfd = other.listenfd;
		other.listenfd = -1;
		return *this;
	}

	const Socket& TCPServer::get_socket() const
	{
		return self_socket;
	}

	TCP TCPServer::accept() const
	{
		sockaddr_in cliaddr{};
		socklen_t clilen = sizeof(cliaddr);

		auto sockfd = ::accept(listenfd, (sockaddr*)&cliaddr, &clilen);
		struct sockaddr_in self_addr {};
		socklen_t len = sizeof(self_addr);
		if (getsockname(sockfd, (struct sockaddr*)&self_addr, &len) < 0)
		{
			auto err = strerror(errno);
			throw "getsockname: " + std::string(err);
		}

		return TCP(sockfd, SocketPair{ self_addr, cliaddr });
	}

	void TCPServer::close()
	{
		if (listenfd > -1)
			::close(listenfd);
		listenfd = -1;
	}

	TCPServer::~TCPServer()
	{
		this->close();
	}
}

namespace Network
{
	TCP::TCP(int sockfd, const SocketPair& sp)
		: sockfd(sockfd), socket_pair(sp)
	{
	}

	TCP::TCP(const SocketPair& sp)
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("socket: " + std::string(err));
		}

		// bind locally. If this information is default, kernel will choose automatically
		struct sockaddr_in serv_addr {};
		explicit_bzero((char*)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = sp.self.ip.get_ip();
		serv_addr.sin_port = sp.self.port.get_port();

		if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("bind: " + std::string(err));
		}

		// Make connection with server
		serv_addr.sin_addr.s_addr = sp.remote.ip.get_ip();
		serv_addr.sin_port = sp.remote.port.get_port();
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("connect: " + std::string(err));
		}

		// Fill the socket pair details. 
		// This can be different from input socket details because of INADDR_ANY and Ephemeral port.
		this->socket_pair.remote = sp.remote;
		struct sockaddr_in addr {};
		socklen_t len = sizeof(addr);
		if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("getsockname: " + std::string(err));
		}
		this->socket_pair.self = addr;
	}	

	TCP::TCP(TCP&& other)
	{
		this->sockfd = other.sockfd;
		this->socket_pair = other.socket_pair;
		other.sockfd = -1;
		other.socket_pair = {};
	}

	TCP& TCP::operator=(TCP&& other)
	{
		if (&other == this)
			return *this;

		this->sockfd = other.sockfd;
		this->socket_pair = other.socket_pair;
		other.sockfd = -1;
		other.socket_pair = {};
		return *this;
	}

	int TCP::send(const std::string& str) const
	{
		std::clog << "[" << this->socket_pair << "]: Sending via TCP" << std::endl;
		hex_view(str.data(), str.size());

		ssize_t nwritten{};
		size_t nleft = str.size();
		const char* ptr = str.data();

		while (nleft > 0)
		{
			if ((nwritten = ::write(this->sockfd, ptr, nleft)) <= 0)
			{
				if (nwritten < 0 && errno == EINTR)
					nwritten = 0;
				else
				{
					auto err = strerror(errno);
					throw std::runtime_error("write: " + std::string(err));
				}
			}

			nleft -= nwritten;
			ptr += nwritten;
		}

		return str.size();
	}

	std::string TCP::receive(size_t n) const
	{
		if (n == 0)
		{
			std::string str(READ_MAX_SIZE, '\0');
			ssize_t n = 0;
			if ((n = ::read(this->sockfd, str.data(), READ_MAX_SIZE)) < 0)
			{
				auto err = strerror(errno);
				throw std::runtime_error("read: " + std::string(err));
			}

			str.resize(n);
			
			std::clog << "[" << this->socket_pair << "]: Received via TCP" << std::endl;
			hex_view(str.data(), str.size());

			return str;
		}

		std::string buffer(n, '\0');
		ssize_t nread{};
		size_t nleft = n;
		char* ptr = buffer.data();

		while (nleft > 0)
		{
			if ((nread = ::read(this->sockfd, ptr, nleft)) < 0)
			{
				if (errno == EINTR)
					nread = 0;
				else
				{
					auto err = strerror(errno);
					throw std::runtime_error("read: " + std::string(err));
				}
			}
			else if (nread == 0)
				break;

			nleft -= nread;
			ptr += nread;
		}

		std::clog << "[" << this->socket_pair << "]: Received via TCP" << std::endl;
		hex_view(buffer.data(), buffer.size());

		return buffer;
	}

	const SocketPair& TCP::get_socket_pair() const
	{
		return socket_pair;
	}

	void TCP::close()
	{
		if (sockfd > -1)
			::close(sockfd);
		sockfd = -1;
	}

	TCP::~TCP()
	{
		this->close();
	}
}

namespace Network
{
	UDP::UDP()
	{
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("socket: " + std::string(err));
		}
	}

	UDP::UDP(const Socket& self_socket) : UDP()
	{
		struct sockaddr_in serv_addr {};
		explicit_bzero((char*)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = self_socket.ip.get_ip();
		serv_addr.sin_port = self_socket.port.get_port();

		if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("bind: " + std::string(err));
		}
	}

	UDP::UDP(const PORT& self_port) : UDP(Socket{ {}, self_port })
	{
	}

	UDP::UDP(UDP&& other)
	{
		this->sockfd = other.sockfd;
		other.sockfd = -1;
	}

	UDP& UDP::operator=(UDP&& other)
	{
		if (this == &other)
			return *this;

		this->sockfd = other.sockfd;
		other.sockfd = -1;
		return *this;
	}

	void UDP::send(const std::string& sv, const Socket& remote) const
	{
		sockaddr_in serv_addr = remote;
		
		std::clog << "[" << remote << "]: Sending via UDP" << std::endl;
		hex_view(sv.data(), sv.size());

		if (sendto(sockfd, sv.data(), sv.size(), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("sendto: " + std::string(err));
		}
	}

	std::pair<std::string, Socket> UDP::receive() const
	{
		sockaddr_in serv_addr{};
		socklen_t len = sizeof(serv_addr);
		std::string str(READ_MAX_SIZE, '\0');

		ssize_t n = 0;
		if ((n = recvfrom(sockfd, str.data(), READ_MAX_SIZE, 0, (sockaddr*)&serv_addr, &len)) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("recvfrom: " + std::string(err));
		}

		str.resize(n);
		
		std::clog << "[" << Socket{ serv_addr } << "]: Received via UDP" << std::endl;
		hex_view(str.data(), str.size());

		return { str, Socket{ serv_addr } };
	}

	const PORT UDP::get_self_port() const
	{
		struct sockaddr_in addr {};
		socklen_t len = sizeof(addr);
		if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0)
		{
			auto err = strerror(errno);
			throw std::runtime_error("getsockname: " + std::string(err));
		}
		return addr.sin_port;
	}

	void UDP::close()
	{
		if (sockfd > -1)
			::close(sockfd);
		sockfd = -1;
	}

	UDP::~UDP()
	{
		this->close();
	}
}