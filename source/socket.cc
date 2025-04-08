#include "socket.h"
#include "async.h"
#include "macro.h"
#include "tools/sockfdinfo.h"

namespace webserver {

static constexpr const char* g_logger = "system";

Socket::Socket(Domain domain, SockType socktype, Protocol protocol)
	: m_closed(true), m_sockfd(-1), m_domain(domain)
	, m_socktype(socktype), m_protocol(protocol)
	, m_localAddress{nullptr}, m_remoteAddress{nullptr} {
	int fd = async::Socket(static_cast<int>(m_domain)
			, static_cast<int>(m_socktype), static_cast<int>(m_protocol));
	if(WEBSERVER_UNLIKELY(fd < 0)) {
		throw std::invalid_argument("Socket::Socket -- socket error"); 
	}
	m_sockfd = fd;
	m_closed = false;
}

Socket::Socket(int sockfd, Domain domain, SockType socktype, Protocol protocol)
	: m_closed(false), m_sockfd(sockfd), m_domain(domain)
	, m_socktype(socktype), m_protocol(protocol)
	, m_localAddress{nullptr}, m_remoteAddress{nullptr} {
}

Socket::~Socket() {
	if(!m_closed) {
		Socket::close();
	}
}

Socket::ptr Socket::CreateIPv4TCPSocket() {
	return std::make_shared<Socket>(Domain::IPv4, SockType::TCP);
}

Socket::ptr Socket::CreateIPv4UDPSocket() {
	return std::make_shared<Socket>(Domain::IPv4, SockType::UDP);
}

Socket::ptr Socket::CreateIPv6TCPSocket() {
	return std::make_shared<Socket>(Domain::IPv6, SockType::TCP);
}

Socket::ptr Socket::CreateIPv6UDPSocket() {
	return std::make_shared<Socket>(Domain::IPv6, SockType::UDP);
}

Socket::ptr Socket::CreateUnixTCPSocket() {
	return std::make_shared<Socket>(Domain::UNIX, SockType::TCP);
}

Socket::ptr Socket::CreateUnixUDPSocket() {
	return std::make_shared<Socket>(Domain::UNIX, SockType::TCP);
}

int Socket::bind(Address::ptr addr) {
	m_localAddress = addr;
	return async::Bind(m_sockfd, addr->getSockaddr(), addr->getLength());
}

int Socket::listen(int backlog) {
	return async::Listen(m_sockfd, backlog);
}

Socket::ptr Socket::accept(Address::ptr& addr) {
	if(!addr) {
		addr = Address::Create(m_domain);
	}
	socklen_t len = addr->getLength();
	int client_sockfd = async::Accept(m_sockfd, addr->getSockaddr(), &len);
	if(client_sockfd < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Socket::accept(addr) error" << strerror(errno);
		addr.reset();
		return nullptr;
	}

	Socket::ptr client_sock = 
			std::make_shared<Socket>(client_sockfd, m_domain, m_socktype, m_protocol);
	client_sock->m_localAddress = m_localAddress;
	client_sock->m_remoteAddress = addr;

	return client_sock;
}

Socket::ptr Socket::accept() {
	Address::ptr addr = Address::Create(m_domain);
	socklen_t len = addr->getLength();

	int client_sockfd = async::Accept(m_sockfd, addr->getSockaddr(), &len);
	if(client_sockfd < 0) {
		return nullptr;
	}

	Socket::ptr client_sock = 
			std::make_shared<Socket>(client_sockfd, m_domain, m_socktype, m_protocol);
	client_sock->m_localAddress = m_localAddress;
	client_sock->m_remoteAddress = addr;

	return client_sock;
}

int Socket::connect_with_timeout(Address::ptr addr, uint64_t timeout) {
	int rt = async::Connect_with_timeout(m_sockfd
			, addr->getSockaddr(), addr->getLength(), timeout);
	if(rt < 0) {
		return -1;
	}
	m_localAddress = Address::Create(m_domain);
	socklen_t len = m_localAddress->getLength();
	int res = getsockname(m_sockfd, m_localAddress->getSockaddr(), &len);
	if(WEBSERVER_UNLIKELY(res < 0)) {
		LIBFUN_ABORT_LOG(errno, "Socket::connect_with_timeout --getsockname  error")
	}

	m_remoteAddress = addr;
	return rt;
}

int Socket::connect(Address::ptr addr) {
	int rt = async::Connect(m_sockfd, addr->getSockaddr(), addr->getLength());
	if(rt < 0) {
		return -1;
	}
	m_localAddress = Address::Create(m_domain);
	socklen_t len = m_localAddress->getLength();
	int res = getsockname(m_sockfd, m_localAddress->getSockaddr(), &len);
	if(WEBSERVER_UNLIKELY(res < 0)) {
		LIBFUN_ABORT_LOG(errno, "Socket::connect_with_timeout --getsockname  error")
	}

	m_remoteAddress = addr;
	return rt;
}

int Socket::close() {
	int rt = async::Close(m_sockfd);
	if(WEBSERVER_UNLIKELY(rt < 0)) {
		return -1;
	}
	m_closed = true;
	m_sockfd = -1;
	m_domain = Domain::ERROR;
	m_socktype = SockType::ERROR;
	m_protocol = Protocol::ERROR;
	m_localAddress = nullptr;
	m_remoteAddress = nullptr;
	
	return rt;
}

//read related
ssize_t Socket::read(void *buf, size_t count) {
	return async::Read(m_sockfd, buf, count);
}

ssize_t Socket::readv(const struct iovec *iov, int iovcnt) {
	return async::Readv(m_sockfd, iov, iovcnt);
}

ssize_t Socket::recv(void *buf, size_t len, int flags) {
	return async::Recv(m_sockfd, buf, len, flags);
}

ssize_t Socket::recvfrom(void *buf, size_t len, Address::ptr& src_addr, int flags) {
	if(!src_addr) {
		src_addr = Address::Create(m_domain);
	}
	socklen_t len2 = src_addr->getLength();
	return async::Recvfrom(m_sockfd, buf, len, flags, src_addr->getSockaddr(), &len2);
}

ssize_t Socket::recvmsg(struct msghdr *msg, int flags) {
	return async::Recvmsg(m_sockfd, msg, flags);
}

//write related
ssize_t Socket::write(const void *buf, size_t count) {
	return async::Write(m_sockfd, buf, count);
}

ssize_t Socket::writev(const struct iovec *iov, int iovcnt) {
	return async::Writev(m_sockfd, iov, iovcnt);
}

ssize_t Socket::send(const void *buf, size_t len, int flags) {
	return async::Send(m_sockfd, buf, len, flags);
}

ssize_t Socket::sendto(const void *buf, size_t len, Address::ptr dest_addr, int flags) {
	if(!dest_addr) {
		errno = EINVAL;
		return -1;
	}
	return async::Sendto(m_sockfd, buf, len
			, flags, dest_addr->getSockaddr(), dest_addr->getLength());
}

ssize_t Socket::sendmsg(const struct msghdr *msg, int flags) {
	return async::Sendmsg(m_sockfd, msg, flags);
}

//other related
void Socket::setBlock() {
	int flags = async::Fcntl(m_sockfd, F_GETFL, 0);
	async::Fcntl(m_sockfd, F_SETFL, flags & ~O_NONBLOCK);
}

void Socket::setNonBlock() {
	int flags = async::Fcntl(m_sockfd, F_GETFL, 0);
	async::Fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
}

int Socket::setAddrReuse() {
	int flag = 1;
	return async::Setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
}

void Socket::setRecvTimeout(uint64_t v) {
	struct timeval tv;
	tv.tv_sec = v / 1000;
	tv.tv_usec = (v % 1000) * 1000;
	async::Setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void Socket::setSendTimeout(uint64_t v) {
	struct timeval tv;
	tv.tv_sec = v / 1000;
	tv.tv_usec = (v % 1000) * 1000;
	async::Setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

uint64_t Socket::getRecvTimeout() {
	struct timeval tv;
	socklen_t len = sizeof(tv);
	memset(&tv, 0x00, sizeof(tv));
	
	async::Getsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &len);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t Socket::getSendTimeout() {
	struct timeval tv;
	socklen_t len = sizeof(tv);
	memset(&tv, 0x00, sizeof(tv));
	
	async::Getsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, &len);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int Socket::setsockopt(int optname, const void *optval, socklen_t optlen) {
	return async::Setsockopt(m_sockfd, SOL_SOCKET, optname, optval, optlen);
}

int Socket::getsockopt(int optname, void *optval, socklen_t *optlen) {
	return async::Getsockopt(m_sockfd, SOL_SOCKET, optname, optval, optlen);
}

std::string Socket::getErrorMsg() const {
	return ::strerror(errno);
}

}	// namespace webserver
