#include "webserver_socket.h"
#include "webserver_socktypes.h"
#include "webserver_log.h"
#include "webserver_exception.h"
#include "webserver_asynapi.h"
#include "webserver_macro.h"
#include "webserver_time.h"
#include "webserver_config.h"
#include "webserver_epollio.h"

namespace webserver {

constexpr const char* g_logger = "system";

uint64_t Socket::MAX_TIMEOUT = ~0ull;

Socket::Socket(SockFamily family, SockType socktype, SockProto proto)
	: m_sockfd(-1), m_family(family), m_sockType(socktype)
	, m_protocol(proto), m_isClosed(true)
	, m_isConnected(false), m_recvTimeout(~0ull)
	, m_sendTimeout(~0ull), m_remoteAddress{}
	, m_localAddress{} {
	
	m_sockfd = asynapi::Socket(m_family, m_sockType, m_protocol);
	if(WEBSERVER_UNLIKELY(m_sockfd < 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Socket::Socket error  what :" << strerror(errno);
		m_family = SockFamily::FA_ERROR;
		m_protocol = SockProto::PR_ERROR;
		m_sockType = SockType::TY_ERROR;
	} else {
		m_isClosed = false;
	}
}

Socket::~Socket() {
	if(!m_isClosed) {
		close();
	}
}

void Socket::invalid() {
	m_sockfd = -1;
	m_family = SockFamily::FA_ERROR;
	m_sockType = SockType::TY_ERROR;
	m_protocol = SockProto::PR_ERROR;
	m_isClosed = true;
	m_isConnected = false;
	m_recvTimeout = MAX_TIMEOUT;
	m_sendTimeout = MAX_TIMEOUT;
	
	m_remoteAddress.reset();
	m_localAddress.reset();
}

bool Socket::listen(int backlog) {
	return asynapi::Listen(m_sockfd, backlog) == 0;
}

bool Socket::bind(Address::ptr addr) {
	int32_t rt = asynapi::Bind(m_sockfd, addr->getSockaddr(), addr->getLenth());
	if(WEBSERVER_LIKELY(rt == 0)) {
		m_localAddress = addr;
		return true;
	}
	return false;
}

Address::ptr Socket::accept() {
	Address::ptr addr = Address::Create(m_family);
	socklen_t len = addr->getLenth();
	int32_t rt = asynapi::Accept(m_sockfd, addr->getSockaddr(), &len);
	if(WEBSERVER_UNLIKELY(rt < 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Socket::Accept error  what :" << strerror(errno);
		return nullptr;
	}
	m_isConnected = true;
	m_remoteAddress = addr;
	return addr;
}

bool Socket::connect(Address::ptr addr) {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << Scheduler::m_currScheduler;

	int32_t rt = asynapi::Connect(m_sockfd, addr->getSockaddr(), addr->getLenth());
	if(WEBSERVER_LIKELY(rt == 0)) {
		m_remoteAddress = addr;
	} else {
		return false;
	}
	
	Address::ptr local_addr = Address::Create(m_family);
	socklen_t len = local_addr->getLenth();
	int rt2 = getsockname(m_sockfd, local_addr->getSockaddr(), &len);
	if(WEBSERVER_UNLIKELY(rt2 < 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Socket::connect --- getsockname error  what :" << strerror(errno);
		throw std::runtime_error("getsockname error");
		return false;
	}
	m_localAddress = local_addr;
	
	return true;
}

bool Socket::connect_timeout(Address::ptr addr, uint64_t connect_timeout) {
	int32_t rt = asynapi::ConnectTimeout(m_sockfd
			, addr->getSockaddr(), addr->getLenth(), connect_timeout);
	if(WEBSERVER_LIKELY(rt) == 0) {
		m_remoteAddress = addr;
	} else {
		return false;
	}
	
	Address::ptr local_addr = Address::Create(m_family);
	socklen_t len = local_addr->getLenth();
	int rt2 = getsockname(m_sockfd, local_addr->getSockaddr(), &len);
	if(WEBSERVER_UNLIKELY(rt2 < 0)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Socket::connect_timeout --- getsockname error  what :" << strerror(errno);
		throw std::runtime_error("getsockname error");
		return false;
	}	
	m_localAddress = local_addr;

	return true;
}

bool Socket::close() {
	int32_t rt = asynapi::Close(m_sockfd);
	if(WEBSERVER_LIKELY(rt == 0)) {
		invalid();
	}
	return rt == 0;
}

ssize_t Socket::read(void* buff, size_t count) {
	return asynapi::Read(m_sockfd, buff, count);
}

ssize_t Socket::readv(const iovec* iov, int iovcnt) {
	return asynapi::Readv(m_sockfd, iov, iovcnt);
}

ssize_t Socket::recv(void* buff, size_t len, MgsFlags flags) {
	return asynapi::Recv(m_sockfd, buff, len, flags);
}

ssize_t Socket::recvFrom(void* buff, size_t len, Address::ptr src_addr, MgsFlags flags) {
	socklen_t len2 = src_addr->getLenth();
	return asynapi::Recvfrom(m_sockfd, buff, len, flags, src_addr->getSockaddr(), &len2);
}

ssize_t Socket::recvMsg(struct msghdr *msg, MgsFlags flags) {
	return asynapi::Recvmsg(m_sockfd, msg, flags);
}

ssize_t Socket::write(const void *buf, size_t count) {
	return asynapi::Write(m_sockfd, buf, count);
}

ssize_t Socket::writev(int fd, const struct iovec *iov, int iovcnt) {
	return asynapi::Writev(m_sockfd, iov, iovcnt);
}

ssize_t Socket::send(const void *buf, size_t len, MgsFlags flags) {
	return asynapi::Send(m_sockfd, buf, len, flags);
}

ssize_t Socket::sendto(const void *buf, size_t len, Address::ptr dst_addr, MgsFlags flags) {
	socklen_t len2 = dst_addr->getLenth();
	return asynapi::Sendto(m_sockfd, buf, len, flags, dst_addr->getSockaddr(), len2);
}

ssize_t Socket::sendmsg(const struct msghdr *msg, MgsFlags flags) {
	return asynapi::Sendmsg(m_sockfd, msg, flags);
}

void Socket::setBolck() {
	int flags = asynapi::Fcntl(m_sockfd, F_GETFL, 0);
	asynapi::Fcntl(m_sockfd, F_SETFL, flags & ~O_NONBLOCK);
}

void Socket::setNonBlock() {
	int flags = asynapi::Fcntl(m_sockfd, F_GETFL, 0);
	asynapi::Fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
}

void Socket::setRecvTimeout(uint64_t ms) {
	Timeval::ptr tmpv = std::make_shared<Timeval>(ms / 1000, (ms % 1000) * 1000);
	asynapi::Setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, tmpv->get(), tmpv->getLenth());
}

void Socket::setSendTimeout(uint64_t ms) {
	Timeval::ptr tmpv = std::make_shared<Timeval>(ms / 1000, (ms % 1000) * 1000);
	asynapi::Setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, tmpv->get(), tmpv->getLenth());
}

uint64_t Socket::getDefConTimeout() const {
	static ConfigVar<Map<std::string, uint32_t> >::ptr 	
		tcp_connect_timeout = ConfigSingleton::GetInstance()->
			getConfigVar<Map<std::string, uint32_t> >("asynapi");

	if(tcp_connect_timeout != nullptr) {
		return tcp_connect_timeout->getVariable().at("tcp_connect_timeout");
	}

	return 5000;
}

}	// namespace webserver
