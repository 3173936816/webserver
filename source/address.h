#ifndef __WEBSERVER_ADDRESS_H__
#define __WEBSERVER_ADDRESS_H__

#include <memory>
#include <vector>
#include <map>

#include "socktypes.h"

namespace webserver {

class Address {
public:
	using ptr = std::shared_ptr<Address>;
	
	Address();
	virtual ~Address();
	
	static Address::ptr Create(Domain domain);
	
	virtual const struct sockaddr* getSockaddr() const = 0;
	virtual struct sockaddr* getSockaddr() = 0;
	virtual std::string visual() const = 0;
	virtual uint32_t getLength() const = 0;

	Domain getDomain() const;
};


class IPAddress : public Address {
public:
	using ptr = std::shared_ptr<IPAddress>;
	
	IPAddress();
	virtual ~IPAddress();
	
	static IPAddress::ptr Create(Domain domain);
	
	static void ResoluteDomainName(std::vector<IPAddress::ptr>& vec
			, const std::string& host, const std::string& server
			, Domain domain = Domain::ANY, SockType type = SockType::ANY
			, Protocol protocol = Protocol::ANY);

	static void GetIfAddresses(
			std::multimap<std::string, std::pair<IPAddress::ptr, uint32_t> >& ifaddresses
			, Domain domain = Domain::ANY, bool need_loopback = false);
	
	virtual IPAddress::ptr getBoradcast(uint32_t prefix_len) const = 0;
	virtual IPAddress::ptr getNetworkSegment(uint32_t prefix_len) const = 0;
	virtual IPAddress::ptr getSubnetMask(uint32_t prefix_len) const = 0;
	
	void setPort(uint16_t port);
	uint16_t getPort() const;
};


class IPv4Address final : public IPAddress {
public:
	using ptr = std::shared_ptr<IPv4Address>;
	
	IPv4Address();
	IPv4Address(const struct sockaddr_in& addrin);
	IPv4Address(const uint8_t(&addr)[4], uint16_t port);
	IPv4Address(const std::string& addr, uint16_t port);
	~IPv4Address();
	
	static void ResoluteAllTCP(std::vector<IPv4Address::ptr>& vec
			, const std::string& host, const std::string& server);
	static void ResoluteAllUDP(std::vector<IPv4Address::ptr>& vec
			, const std::string& host, const std::string& server);
	
	static IPv4Address::ptr ResoluteAnyTCP(const std::string& host
			, const std::string& server);
	static IPv4Address::ptr ResoluteAnyUDP(const std::string& host
			, const std::string& server);
	
	static void GetAllIfAddresses(std::multimap<std::string
			, std::pair<IPv4Address::ptr, uint32_t> >& ifaddresses);
	
	static std::pair<IPv4Address::ptr, uint32_t> GetAnyIfAddress();

	const struct sockaddr* getSockaddr() const override;
	struct sockaddr* getSockaddr() override;
	std::string visual() const override;
	uint32_t getLength() const override;
	
	IPAddress::ptr getBoradcast(uint32_t prefix_len) const override;
	IPAddress::ptr getNetworkSegment(uint32_t prefix_len) const override;
	IPAddress::ptr getSubnetMask(uint32_t prefix_len) const override;

private:
	struct sockaddr_in m_addrin;
};


class IPv6Address final : public IPAddress {
public:
	using ptr = std::shared_ptr<IPv6Address>;
	
	IPv6Address();
	IPv6Address(const struct sockaddr_in6& addrin6);
	IPv6Address(const uint8_t(&addr)[16], uint16_t port);
	IPv6Address(const std::string& addr, uint16_t port);
	~IPv6Address();
	
	static void ResoluteAllTCP(std::vector<IPv6Address::ptr>& vec
			, const std::string& host, const std::string& server);
	static void ResoluteAllUDP(std::vector<IPv6Address::ptr>& vec
			, const std::string& host, const std::string& server);
	
	static IPv6Address::ptr ResoluteAnyTCP(const std::string& host
			, const std::string& server);
	static IPv6Address::ptr ResoluteAnyUDP(const std::string& host
			, const std::string& server);
	
	static void GetAllIfAddresses(std::multimap<std::string
			, std::pair<IPv6Address::ptr, uint32_t> >& ifaddresses);
	
	static std::pair<IPv6Address::ptr, uint32_t> GetAnyIfAddress();
	
	const struct sockaddr* getSockaddr() const override;
	struct sockaddr* getSockaddr() override;
	std::string visual() const override;
	uint32_t getLength() const override;
	
	IPAddress::ptr getBoradcast(uint32_t prefix_len) const override;
	IPAddress::ptr getNetworkSegment(uint32_t prefix_len) const override;
	IPAddress::ptr getSubnetMask(uint32_t prefix_len) const override;

private:
	struct sockaddr_in6 m_addrin6;
};


class UnixAddress final : public Address {
public:
	using ptr = std::shared_ptr<UnixAddress>;
	
	UnixAddress();
	UnixAddress(const struct sockaddr_un& addrun);
	UnixAddress(const std::string& path);
	~UnixAddress();
	
	const struct sockaddr* getSockaddr() const override;
	struct sockaddr* getSockaddr() override;
	std::string visual() const override;
	uint32_t getLength() const override;
	uint32_t getActualLength() const;

private:
	struct sockaddr_un m_addrun;
};

}	// namespace webserver

#endif // ! __WEBSERVER_ADDRESS_H__
