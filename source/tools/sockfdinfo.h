#ifndef __WEBSERVER_SOCKFDINFO_H__
#define __WEBSERVER_SOCKFDINFO_H__

#include <memory>
#include <stdint.h>
#include <unordered_map>

#include "../synchronism.h"
#include "../singleton.hpp"

namespace webserver {

class SockFdInfo {
	friend class SockFdInfoManager;
public:
	using ptr = std::shared_ptr<SockFdInfo>;
	using MutexType = RWMutex;
	
	~SockFdInfo();
	
	int getSockfd() const;
	bool isClosed() const;
	uint64_t getRecvTimeout() const; 
	uint64_t getSendTimeout() const; 
	bool getUserNonBlock() const; 
	
	void setRecvTimeout(uint64_t v);
 	void setSendTimeout(uint64_t v);
	void setUserNonBlock(bool v);
	void setClose(bool v);

private:
	SockFdInfo(int fd);

private:
	int m_fd;
	bool m_close : 1;
	uint64_t m_recvTimeout;
	uint64_t m_sendTimeout;
	bool m_userNonBlock : 1;
	mutable MutexType m_rwmtx;
};


class SockFdInfoManager {
public:
	using ptr = std::shared_ptr<SockFdInfoManager>;
	using MutexType = Mutex;

	SockFdInfoManager();
	~SockFdInfoManager();
	
	SockFdInfo::ptr addSockFdInfo(int sockfd);
	SockFdInfo::ptr getSockFdInfo(int sockfd);
	void delSockFdInfo(int sockfd);
	void clear();

private:
	MutexType m_mtx;
	std::unordered_map<int, SockFdInfo::ptr> m_sockFdInfoes;
};

using SockFdInfoMgr = Singleton<SockFdInfoManager>;

}

#endif // ! __WEBSERVER_SOCKFDINFO_H__
