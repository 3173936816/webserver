#ifndef __WEBSERVER_SOCKET_H__
#define __WEBSERVER_SOCKET_H__

#include <memory>
#include <stdint.h>

#include "address.h"

namespace webserver {

class Socket {
public:
	using ptr = std::shared_ptr<Socket>;

	Socket(Domain domain, SockType socktype, Protocol protocol = Protocol::ANY);
	Socket(int sockfd, Domain domain, SockType socktype, Protocol protocol);
	~Socket();
	
	static Socket::ptr CreateIPv4TCPSocket();
	static Socket::ptr CreateIPv4UDPSocket();
	
	static Socket::ptr CreateIPv6TCPSocket();
	static Socket::ptr CreateIPv6UDPSocket();
	
	static Socket::ptr CreateUnixTCPSocket();
	static Socket::ptr CreateUnixUDPSocket();
	
	int bind(Address::ptr addr);
	int listen(int backlog);
	Socket::ptr accept(Address::ptr& addr);
	Socket::ptr accept();
	int connect_with_timeout(Address::ptr addr, uint64_t timeout);
	int connect(Address::ptr addr);
	int close();

	//read related
	ssize_t read(void *buf, size_t count);	
	ssize_t readv(const struct iovec *iov, int iovcnt);
	ssize_t recv(void *buf, size_t len, int flags = 0);
	ssize_t recvfrom(void *buf, size_t len, Address::ptr& src_addr, int flags = 0);
	ssize_t recvmsg(struct msghdr *msg, int flags = 0);
	
	//write related
	ssize_t write(const void *buf, size_t count);
	ssize_t writev(const struct iovec *iov, int iovcnt);
	ssize_t send(const void *buf, size_t len, int flags = 0);
	ssize_t sendto(const void *buf, size_t len, Address::ptr dest_addr, int flags = 0);
	ssize_t sendmsg(const struct msghdr *msg, int flags = 0);
	
	//other related
	void setBlock();
	void setNonBlock();
	int setAddrReuse();
	void setRecvTimeout(uint64_t v);
	void setSendTimeout(uint64_t v);
	uint64_t getRecvTimeout();
	uint64_t getSendTimeout();
	int setsockopt(int optname, const void *optval, socklen_t optlen);
	int getsockopt(int optname, void *optval, socklen_t *optlen);
	
	int getSockfd() const { return m_sockfd; }
	bool isClosed() const { return m_closed; }
	Domain getDomain() const { return m_domain; }
	SockType getSockType() const { return m_socktype; }
	Protocol getProtocol() const { return m_protocol; }
	Address::ptr getLocalAddress() const { return m_localAddress; }
	Address::ptr getRemoteAddress() const { return m_remoteAddress; }

	std::string getErrorMsg() const;
private:
	bool m_closed;
	int m_sockfd;
	Domain m_domain;
	SockType m_socktype;
	Protocol m_protocol;
	Address::ptr m_localAddress;
	Address::ptr m_remoteAddress;
};

}	// namespace webserver

#endif // ! __WEBSERVER_SOCKET_H__
