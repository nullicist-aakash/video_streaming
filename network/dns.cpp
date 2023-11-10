#include "dns.h"
#include <netdb.h>
#include <iostream>

namespace Network
{
	DNSResponse get_dns_response(std::string_view hostname)
	{
		auto hptr = gethostbyname(hostname.data());
		if (hptr == nullptr)
			throw std::runtime_error(std::string("gethostbyname failed for host ") + std::string(hostname) + ": " + hstrerror(h_errno));

		DNSResponse response;
		response.hostname = hptr->h_name;
		for (auto pptr = hptr->h_aliases; *pptr; ++pptr)
			response.aliases.push_back(*pptr);

		if (hptr->h_addrtype != AF_INET)
			throw std::runtime_error(std::string("gethostbyname returned an unsupported address type for host: ") + std::string(hostname));

		for (auto pptr = hptr->h_addr_list; *pptr; ++pptr)
		{
			uint32_t ip;
			memcpy(&ip, *pptr, sizeof(ip));
			response.ips.push_back(ip);
		}

		return response;
	}
}