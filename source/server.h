#ifndef __WEBSERVER_SERVER_H__
#define __WEBSERVER_SERVER_H__

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <map>

#include "socket.h"
#include "address.h"
#include "epollio.h"
#include "noncopyable.h"

namespace webserver {

class Server : Noncopyable {
public:
	using ptr = std::shared_ptr<Server>;
	
	enum IOFramework {
		SELECT 	= 0,
		POLL 	= 1,
		EPOLL 	= 2
	};
	
	Server(const std::string& name, uint32_t thread_num, IOFramework framework = EPOLL);
	virtual ~Server();
	
	void setWorkDir(const std::string& path);
	void registerSigEvent(std::function<void()> func, int signum);
	
	bool bindTCPAddr(Address::ptr addr);
	bool bindUDPAddr(Address::ptr addr);
	bool bindTCPAddrs(const std::vector<Address::ptr>& addrs
			, std::vector<Address::ptr>& fails);
	bool bindUDPAddrs(const std::vector<Address::ptr>& addrs
			, std::vector<Address::ptr>& fails);
	
	void dispatch();
	void loopExit();
	
	std::string getServerName() const { return m_name; }
	IOFramework getIOFramework() const { return m_IOFramework; }

private:
	void acceptTCPClient(Socket::ptr listener);
	void waitUDPClient(Socket::ptr listener);

protected:
	virtual void doInteracteTCPClient(Socket::ptr client) = 0;
	virtual void doInteracteUDPClient(Socket::ptr client) = 0;

private:
	std::atomic<bool> m_isStop;
	std::string m_name;
	IOFramework m_IOFramework;
	std::vector<Socket::ptr> m_TCPListeners;
	std::vector<Socket::ptr> m_UDPListeners;
	IOBase::ptr m_IOBase;
	
	std::multimap<int, std::function<void()> > m_sigEvents;
};

}	// namsepace webserver

#endif // ! __WEBSERVER_SERVER_H__
