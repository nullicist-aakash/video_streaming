#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>

struct SocketCompare
{
   bool operator() (const sockaddr_in& lhs, const sockaddr_in& rhs) const;
};

class Connection
{
private:
    std::map<std::string, std::vector<sockaddr_in>> identifier_to_sockets;
    std::map<sockaddr_in, std::string, SocketCompare> socket_to_identifier;

public:
    Connection();
    ~Connection();

    void add_entry(sockaddr_in& addr, const std::string& identifier);
    void remove_by_addr(sockaddr_in& addr);
    void remove_by_identifier(const std::string &identifier);

    const std::vector<sockaddr_in> get_sockets_by_identifier(const std::string& identifier) const;
    int get_socket_count_by_identifier(const std::string& identifier) const;
};