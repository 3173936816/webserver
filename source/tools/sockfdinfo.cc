#include "sockfdinfo.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "../macro.h"

namespace webserver {

SockFdInfo::SockFdInfo(int fd)
	: m_fd(fd), m_close(true)
	, m_recvTimeout(~0x0ull)
	, m_sendTimeout(~0x0ull)
	, m_userNonBlock(false)
	, m_rwmtx{} {
}

SockFdInfo::~SockFdInfo() = default;

int SockFdInfo::getSockfd() const { 
	RWMutex::RDLock locker(m_rwmtx);
	return m_fd; 
}

bool SockFdInfo::isClosed() const {
	RWMutex::RDLock locker(m_rwmtx);
	return m_close; 
}

uint64_t SockFdInfo::getRecvTimeout() const { 
	RWMutex::RDLock locker(m_rwmtx);
	return m_recvTimeout; 
}

uint64_t SockFdInfo::getSendTimeout() const { 
	RWMutex::RDLock locker(m_rwmtx);
	return m_sendTimeout; 
}

bool SockFdInfo::getUserNonBlock() const { 
	RWMutex::RDLock locker(m_rwmtx);
	return m_userNonBlock; 
}

void SockFdInfo::setRecvTimeout(uint64_t v) { 
	RWMutex::WRLock locker(m_rwmtx);
	m_recvTimeout = v; 
}

void SockFdInfo::setSendTimeout(uint64_t v) { 
	RWMutex::WRLock locker(m_rwmtx);
	m_sendTimeout = v; 
}

void SockFdInfo::setUserNonBlock(bool v) { 
	RWMutex::WRLock locker(m_rwmtx);
	m_userNonBlock = v; 
}

void SockFdInfo::setClose(bool v) {
	RWMutex::WRLock locker(m_rwmtx);
	if(v) {
		m_close = true;
		m_recvTimeout = ~0x0ull;
		m_sendTimeout = ~0x0ull;
		m_userNonBlock = false;
	} else {
		m_close = false;
	}
}

SockFdInfoManager::SockFdInfoManager()
	: m_mtx{Mutex::CHECK}
	, m_sockFdInfoes{} {
}

SockFdInfoManager::~SockFdInfoManager() = default;

SockFdInfo::ptr SockFdInfoManager::addSockFdInfo(int sockfd) {
	struct stat st;
	int32_t rt = fstat(sockfd, &st);
	if(WEBSERVER_UNLIKELY(rt < 0)) {
		LIBFUN_ABORT_LOG(errno, "SockFdInfo -- fstat error");
	}
	
	if(!S_ISSOCK(st.st_mode)) {
		return nullptr;
	}	
	
	int flag = ::fcntl(sockfd, F_GETFL, 0);
	::fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);

	Mutex::Lock locker(m_mtx);
	auto it = m_sockFdInfoes.find(sockfd);
	if(it == m_sockFdInfoes.end()) {
		m_sockFdInfoes[sockfd].reset(new SockFdInfo(sockfd));
	} else if(!it->second->isClosed()) {
		return nullptr;
	}
	m_sockFdInfoes[sockfd]->setClose(false);

	return m_sockFdInfoes[sockfd];
}

SockFdInfo::ptr SockFdInfoManager::getSockFdInfo(int sockfd) {
	Mutex::Lock locker(m_mtx);
	auto it = m_sockFdInfoes.find(sockfd);
	if(it == m_sockFdInfoes.end()
		|| it->second->isClosed()) {
		return nullptr;
	}
	return it->second;
}

void SockFdInfoManager::delSockFdInfo(int sockfd) {
	Mutex::Lock locker(m_mtx);
	m_sockFdInfoes.erase(sockfd);
}

void SockFdInfoManager::clear() {
	Mutex::Lock locker(m_mtx);
	m_sockFdInfoes.clear();
}

} // namespace webserver
