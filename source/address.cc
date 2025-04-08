#include "address.h"
#include "log.h"
#include "macro.h"
#include "endian_convert.h"

#include <arpa/inet.h>
#include <sstream>
#include <exception>
#include <iomanip>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

namespace webserver {

static constexpr const char* g_logger = "system";

template <typename T>
static T CreateMask(uint32_t prefix_len) {
	if(prefix_len > sizeof(T) * 8) {
		throw std::out_of_range("invalid prefix_len");
	}
	return ~((static_cast<T>(0x01) << (sizeof(T) * 8 - prefix_len)) - 1);
}

template <typename T>
static uint32_t ByteCount(T value) {
	uint32_t count = 0;
	for(; value; ++count) {
		value &= value - 1;
	}
	return count;
}

//	Address
Address::Address() = default;
Address::~Address() = default;

Address::ptr Address::Create(Domain domain) {
	if(Domain::IPv4 == domain) {
		return std::make_shared<IPv4Address>();
	}
	if(Domain::IPv6 == domain) {
		return std::make_shared<IPv6Address>();
	}
	if(Domain::UNIX == domain) {
		return std::make_shared<UnixAddress>();
	}
	return nullptr;
}

Domain Address::getDomain() const {
	const struct sockaddr* addr = getSockaddr();

	if(addr->sa_family == AF_INET) { return Domain::IPv4; }
	else if(addr->sa_family == AF_INET6) { return Domain::IPv6; }
	else if(addr->sa_family == AF_UNIX) { return Domain::UNIX; }
	
	return Domain::ERROR;
}

//	IPAddress
IPAddress::IPAddress()
	: Address() {
}

IPAddress::~IPAddress() = default;

IPAddress::ptr IPAddress::Create(Domain domain) {
	if(Domain::IPv4 == domain) {
		return std::make_shared<IPv4Address>();
	}
	if(Domain::IPv6 == domain) {
		return std::make_shared<IPv6Address>();
	}
	return nullptr;
}

void IPAddress::ResoluteDomainName(std::vector<IPAddress::ptr>& vec
		, const std::string& host, const std::string& server
		, Domain domain, SockType type, Protocol protocol) {
	struct addrinfo hints, *res = nullptr;
	memset(&hints, 0x00, sizeof(hints));
	
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
	hints.ai_family = static_cast<int>(domain);
	hints.ai_socktype = static_cast<int>(type);
	hints.ai_protocol = static_cast<int>(protocol);
	
	int32_t rt = getaddrinfo(host.c_str(), server.c_str(), &hints, &res);
	if(WEBSERVER_UNLIKELY(rt != 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "IPAddress::ResoluteDomainName --- getaddrinfo error"
			<< " what : " << gai_strerror(rt);
		return;
	}
	
	struct addrinfo* ptr = res;
	for(; ptr != nullptr; ptr = ptr->ai_next) {
		IPAddress::ptr addr = nullptr;
		int family = ptr->ai_addr->sa_family;
		if(family == AF_INET) {
			addr = std::make_shared<IPv4Address>(*(struct sockaddr_in*)(ptr->ai_addr));
		} else if(family == AF_INET6) {
			addr = std::make_shared<IPv6Address>(*(struct sockaddr_in6*)(ptr->ai_addr));
		}
		if(addr) {
			vec.push_back(addr);
		}
	}
	freeaddrinfo(res);
}

void IPAddress::GetIfAddresses(
		std::multimap<std::string, std::pair<IPAddress::ptr, uint32_t> >& ifaddresses
		, Domain domain, bool need_loopback) {
	struct ifaddrs* res = nullptr;

	int32_t rt = getifaddrs(&res);
	if(WEBSERVER_UNLIKELY(rt < 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "IPAddress::GetIfAffresses -- getifaddrs error"
			<< " what : " << strerror(errno);
		return;
	}
	
	struct ifaddrs* ptr = res;
	for(; ptr != nullptr; ptr = ptr->ifa_next) {
		std::string name = ptr->ifa_name;
		IPAddress::ptr addr = nullptr;
		uint32_t prefix_len = 0;

		int family = ptr->ifa_addr->sa_family;
		if(family == AF_INET 
				&& (domain == Domain::ANY || domain == Domain::IPv4)) {

			struct sockaddr_in* addrin = (struct sockaddr_in*)(ptr->ifa_addr);
			addr = std::make_shared<IPv4Address>(*addrin);
			struct sockaddr_in* mask = (struct sockaddr_in*)(ptr->ifa_netmask);
			prefix_len = ByteCount<uint32_t>(mask->sin_addr.s_addr);

		} else if(family == AF_INET6 
				&& (domain == Domain::ANY || domain == Domain::IPv6)) {

			struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)(ptr->ifa_addr);
			addr = std::make_shared<IPv6Address>(*addrin6);
			struct sockaddr_in6* mask = (struct sockaddr_in6*)(ptr->ifa_netmask);
			for(size_t i = 0; i < 16; ++i) {
				prefix_len += ByteCount<uint8_t>(mask->sin6_addr.s6_addr[i]);
			}

		}
		if(addr) {
			ifaddresses.insert({name, {addr, prefix_len}});
		}
	}
	
	freeifaddrs(res);
	
	if(!need_loopback) {
		ifaddresses.erase("lo");
	}
}

void IPAddress::setPort(uint16_t port) {
	struct sockaddr* addr = getSockaddr();	

	if(addr->sa_family == AF_INET) {
		struct sockaddr_in* addrin = (struct sockaddr_in*)addr;
		addrin->sin_port = HostToNetwork16(port);
	} else if(addr->sa_family == AF_INET6) {
		struct sockaddr_in6* addrin6 = (struct sockaddr_in6*)addr;
		addrin6->sin6_port = HostToNetwork16(port);
	}
}

uint16_t IPAddress::getPort() const {
	const struct sockaddr* addr = getSockaddr();
	uint16_t port = 0;
	
	if(addr->sa_family == AF_INET) {
		const struct sockaddr_in* addrin = (const struct sockaddr_in*)addr;
		port = NetworkToHost16(addrin->sin_port);
	} else if(addr->sa_family == AF_INET6) {
		const struct sockaddr_in6* addrin6 = (const struct sockaddr_in6*)addr;
		port = NetworkToHost16(addrin6->sin6_port);
	}
	return port;
}

// IPv4Address
IPv4Address::IPv4Address()
	: IPAddress(), m_addrin{} {
	memset(&m_addrin, 0x00, sizeof(m_addrin));
	m_addrin.sin_family = AF_INET;
}

IPv4Address::IPv4Address(const struct sockaddr_in& addrin)
	: IPAddress(), m_addrin{} {
	m_addrin = addrin;
}

IPv4Address::IPv4Address(const uint8_t(&addr)[4], uint16_t port)
	: IPAddress(), m_addrin{} {
	m_addrin.sin_family = AF_INET;
	m_addrin.sin_port = HostToNetwork16(port);
	memcpy(&m_addrin.sin_addr.s_addr, addr, sizeof(addr));
}

IPv4Address::IPv4Address(const std::string& addr, uint16_t port)
	: IPAddress(), m_addrin{} {
	m_addrin.sin_family = AF_INET;
	m_addrin.sin_port = HostToNetwork16(port);
	inet_pton(AF_INET, addr.c_str(), &m_addrin.sin_addr.s_addr);
}

IPv4Address::~IPv4Address() = default;

void IPv4Address::ResoluteAllTCP(std::vector<IPv4Address::ptr>& vec
		, const std::string& host, const std::string& server) {
	std::vector<IPAddress::ptr> tmp_vec;
	IPAddress::ResoluteDomainName(tmp_vec, host, server
			, Domain::IPv4, SockType::TCP, Protocol::ANY);
	
	for(auto& it : tmp_vec) {
		vec.push_back(std::dynamic_pointer_cast<IPv4Address>(it));
	}
}

void IPv4Address::ResoluteAllUDP(std::vector<IPv4Address::ptr>& vec
		, const std::string& host, const std::string& server) {
	std::vector<IPAddress::ptr> tmp_vec;
	IPAddress::ResoluteDomainName(tmp_vec, host, server
			, Domain::IPv4, SockType::UDP, Protocol::ANY);
	
	for(auto& it : tmp_vec) {
		vec.push_back(std::dynamic_pointer_cast<IPv4Address>(it));
	}
}

IPv4Address::ptr IPv4Address::ResoluteAnyTCP(const std::string& host
		, const std::string& server) {
	std::vector<IPv4Address::ptr> tmp_vec;
	ResoluteAllTCP(tmp_vec, host, server);
	
	if(!tmp_vec.empty()) {
		return tmp_vec[0];
	}
	return nullptr;
}

IPv4Address::ptr IPv4Address::ResoluteAnyUDP(const std::string& host
		, const std::string& server) {
	std::vector<IPv4Address::ptr> tmp_vec;
	ResoluteAllUDP(tmp_vec, host, server);
	
	if(!tmp_vec.empty()) {
		return tmp_vec[0];
	}
	return nullptr;
}

void IPv4Address::GetAllIfAddresses(std::multimap<std::string
			, std::pair<IPv4Address::ptr, uint32_t> >& ifaddresses) {
	std::multimap<std::string, std::pair<IPAddress::ptr, uint32_t> > tmp_ifaddresses;
	GetIfAddresses(tmp_ifaddresses, Domain::IPv4);
	
	for(auto& it : tmp_ifaddresses) {
		IPv4Address::ptr ipv4 = std::dynamic_pointer_cast<IPv4Address>(it.second.first);
		ifaddresses.insert({it.first, {ipv4, it.second.second}});
	}
}

std::pair<IPv4Address::ptr, uint32_t> IPv4Address::GetAnyIfAddress() {
	std::multimap<std::string, std::pair<IPv4Address::ptr, uint32_t> > tmp_ifaddresses;
	GetAllIfAddresses(tmp_ifaddresses);
	
	if(!tmp_ifaddresses.empty()) {
		return tmp_ifaddresses.begin()->second;
	}
	return {nullptr, 0};
}

const struct sockaddr* IPv4Address::getSockaddr() const {
	return (const struct sockaddr*)&m_addrin;
}

struct sockaddr* IPv4Address::getSockaddr() {
	return (struct sockaddr*)&m_addrin;
}

std::string IPv4Address::visual() const {
	std::ostringstream oss;
	uint32_t addr = NetworkToHost32(m_addrin.sin_addr.s_addr);

	oss << ((addr >> 24) & 0xfful) << "."
		<< ((addr >> 16) & 0xfful) << "."
		<< ((addr >> 8) & 0xfful) << "."
		<< ((addr) & 0xfful);
	
	return oss.str();
}

uint32_t IPv4Address::getLength() const {
	return sizeof(m_addrin);
}

IPAddress::ptr IPv4Address::getBoradcast(uint32_t prefix_len) const {
	struct sockaddr_in boradcast_addr = m_addrin;
	boradcast_addr.sin_addr.s_addr |= HostToNetwork32(~CreateMask<uint32_t>(prefix_len));
	
	return std::make_shared<IPv4Address>(boradcast_addr);
}

IPAddress::ptr IPv4Address::getNetworkSegment(uint32_t prefix_len) const {
	struct sockaddr_in segment_addr = m_addrin;
	segment_addr.sin_addr.s_addr &= HostToNetwork32(CreateMask<uint32_t>(prefix_len));
	
	return std::make_shared<IPv4Address>(segment_addr);
}

IPAddress::ptr IPv4Address::getSubnetMask(uint32_t prefix_len) const {
	struct sockaddr_in mask_addr = m_addrin;
	mask_addr.sin_addr.s_addr = HostToNetwork32(CreateMask<uint32_t>(prefix_len));
	
	return std::make_shared<IPv4Address>(mask_addr);
}

//	IPv6Address
IPv6Address::IPv6Address()
	: IPAddress(), m_addrin6{} {
	memset(&m_addrin6, 0x00, sizeof(m_addrin6));
	m_addrin6.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const struct sockaddr_in6& addrin6)
	: IPAddress(), m_addrin6{} {
	m_addrin6 = addrin6;
}

IPv6Address::IPv6Address(const uint8_t(&addr)[16], uint16_t port)
	: IPAddress(), m_addrin6{} {
	m_addrin6.sin6_family = AF_INET6;
	m_addrin6.sin6_port = HostToNetwork16(port);
	memcpy(&m_addrin6.sin6_addr.s6_addr, addr, sizeof(addr));
}

IPv6Address::IPv6Address(const std::string& addr, uint16_t port)
	: IPAddress(), m_addrin6{} {
	m_addrin6.sin6_family = AF_INET6;
	m_addrin6.sin6_port = HostToNetwork16(port);
	inet_pton(AF_INET6, addr.c_str(), &m_addrin6.sin6_addr.s6_addr);
}

IPv6Address::~IPv6Address() = default;

void IPv6Address::ResoluteAllTCP(std::vector<IPv6Address::ptr>& vec
		, const std::string& host, const std::string& server) {
	std::vector<IPAddress::ptr> tmp_vec;
	IPAddress::ResoluteDomainName(tmp_vec, host, server
		, Domain::IPv6, SockType::TCP, Protocol::ANY);
	
	for(auto& it : tmp_vec) {
		vec.push_back(std::dynamic_pointer_cast<IPv6Address>(it));
	} 
}

void IPv6Address::ResoluteAllUDP(std::vector<IPv6Address::ptr>& vec
		, const std::string& host, const std::string& server) {
	std::vector<IPAddress::ptr> tmp_vec;
	IPAddress::ResoluteDomainName(tmp_vec, host, server
		, Domain::IPv6, SockType::UDP, Protocol::ANY);
	
	for(auto& it : tmp_vec) {
		vec.push_back(std::dynamic_pointer_cast<IPv6Address>(it));
	}
}

IPv6Address::ptr IPv6Address::ResoluteAnyTCP(const std::string& host
		, const std::string& server) {
	std::vector<IPv6Address::ptr> tmp_vec;
	ResoluteAllTCP(tmp_vec, host, server);
	
	if(!tmp_vec.empty()) {
		return tmp_vec[0];
	}
	return nullptr;
}

IPv6Address::ptr IPv6Address::ResoluteAnyUDP(const std::string& host
		, const std::string& server) {
	std::vector<IPv6Address::ptr> tmp_vec;
	ResoluteAllUDP(tmp_vec, host, server);
	
	if(!tmp_vec.empty()) {
		return tmp_vec[0];
	}
	return nullptr;
}

void IPv6Address::GetAllIfAddresses(std::multimap<std::string
			, std::pair<IPv6Address::ptr, uint32_t> >& ifaddresses) {
	std::multimap<std::string, std::pair<IPAddress::ptr, uint32_t> > tmp_ifaddresses;
	GetIfAddresses(tmp_ifaddresses, Domain::IPv6);
	
	for(auto& it : tmp_ifaddresses) {
		IPv6Address::ptr ipv6 = std::dynamic_pointer_cast<IPv6Address>(it.second.first);
		ifaddresses.insert({it.first, {ipv6, it.second.second}});
	}
}

std::pair<IPv6Address::ptr, uint32_t> IPv6Address::GetAnyIfAddress() {
	std::multimap<std::string, std::pair<IPv6Address::ptr, uint32_t> > tmp_ifaddresses;
	GetAllIfAddresses(tmp_ifaddresses);
	
	if(!tmp_ifaddresses.empty()) {
		return tmp_ifaddresses.begin()->second;
	}
	return {nullptr, 0};
}


const struct sockaddr* IPv6Address::getSockaddr() const {
	return (const struct sockaddr*)&m_addrin6;
}

struct sockaddr* IPv6Address::getSockaddr() {
	return (struct sockaddr*)&m_addrin6;
}

std::string IPv6Address::visual() const {
	uint8_t buff[INET6_ADDRSTRLEN] = {0};
	inet_ntop(AF_INET6, &m_addrin6.sin6_addr.s6_addr, (char*)buff, sizeof(buff));

	return std::string((char*)buff, strlen((char*)buff));
	
	//uint16_t buff2[8] = {0};
	//memcpy(buff2, &m_addrin6.sin6_addr.s6_addr, sizeof(buff2));
	//std::ostringstream oss;
	//for(size_t i = 0; i < 8; ++i) {
	//	uint16_t tmp = NetworkToHost16(buff2[i]);
	//	oss << std::hex << std::setw(2) << std::setfill('0') << (tmp >> 8)
	//		<< std::hex << std::setw(2) << std::setfill('0') << (tmp & 0xff);
	//	if(i < 7) {
	//		oss << ":";
	//	}
	//}

	//return str;
}

uint32_t IPv6Address::getLength() const {
	return sizeof(m_addrin6);
}

IPAddress::ptr IPv6Address::getBoradcast(uint32_t prefix_len) const {
	struct sockaddr_in6 addrin6 = m_addrin6;
	uint8_t buff[16] = {0};
	memcpy(buff, &addrin6.sin6_addr.s6_addr, sizeof(buff));
	
	buff[prefix_len / 8] |= ~CreateMask<uint8_t>(prefix_len % 8);
	for(size_t i = prefix_len / 8 + 1; i < 16; ++i) {
		buff[i] = 0xff; 
	}
	memcpy(&addrin6.sin6_addr.s6_addr, buff, sizeof(buff));
	
	return std::make_shared<IPv6Address>(addrin6);
}

IPAddress::ptr IPv6Address::getNetworkSegment(uint32_t prefix_len) const {
	struct sockaddr_in6 addrin6 = m_addrin6;
	uint8_t buff[16] = {0};
	memcpy(buff, &addrin6.sin6_addr.s6_addr, sizeof(buff));

	buff[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
	for(size_t i = prefix_len / 8 + 1; i < 16; ++i) {
		buff[i] = 0x00; 
	}
	memcpy(&addrin6.sin6_addr.s6_addr, buff, sizeof(buff));
	
	return std::make_shared<IPv6Address>(addrin6);
}

IPAddress::ptr IPv6Address::getSubnetMask(uint32_t prefix_len) const {
	struct sockaddr_in6 addrin6 = m_addrin6;
	uint8_t buff[16] = {0};
	memset(&addrin6, 0x00, sizeof(addrin6));
	
	buff[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
	for(size_t i = 0; i < prefix_len / 8; ++i) {
		buff[i] = 0xff; 
	}
	memcpy(&addrin6.sin6_addr.s6_addr, buff, sizeof(buff));
	
	return std::make_shared<IPv6Address>(addrin6);
}

static size_t MAX_PATH_LEN = 
		sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path) - 1;

UnixAddress::UnixAddress()
	: Address(), m_addrun{} {
	memset(&m_addrun, 0x00, sizeof(m_addrun));
	m_addrun.sun_family = AF_UNIX;
}

UnixAddress::UnixAddress(const struct sockaddr_un& addrun)
	: Address(), m_addrun{} {
	m_addrun = addrun;
}

UnixAddress::UnixAddress(const std::string& path)
	: Address(), m_addrun{} {
	if(path.size() > MAX_PATH_LEN) {
		throw std::out_of_range("path is too long");
	}

	m_addrun.sun_family = AF_UNIX;
	memcpy(m_addrun.sun_path, path.c_str(), path.size() + 1);
}

UnixAddress::~UnixAddress() = default;

const struct sockaddr* UnixAddress::getSockaddr() const {
	return (const struct sockaddr*)&m_addrun;
}

struct sockaddr* UnixAddress::getSockaddr() {
	return (struct sockaddr*)&m_addrun;
}

std::string UnixAddress::visual() const {
	return std::string(m_addrun.sun_path, strlen(m_addrun.sun_path));
}

uint32_t UnixAddress::getLength() const {
	return sizeof(m_addrun);
}

uint32_t UnixAddress::getActualLength() const {
	return offsetof(struct sockaddr_un, sun_path) + strlen(m_addrun.sun_path) + 1;
}

}	// namespace webserver
