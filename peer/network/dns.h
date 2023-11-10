#include "data_types.h"
#include <vector>
#include <cstring>
#include <string_view>

namespace Network
{
	struct DNSResponse
	{
		std::string hostname;
		std::vector<std::string> aliases;
		std::vector<IP> ips;
	};

	DNSResponse get_dns_response(std::string_view);
}