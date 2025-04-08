#include "server.h"
#include "log.h"
#include "tools/sockfdinfo.h"
#include "config.h"

#include <unistd.h>	
#include <signal.h>
#include <stdlib.h>

namespace webserver {

//server configuration integration
namespace {

static ConfigVar<Vector<Map<std::string, std::string> > >::ptr Server_Config_Var = 
	ConfigSingleton::GetInstance()->addConfigVar<Vector<Map<std::string, std::string> > >("server", 
		Vector<Map<std::string, std::string> >{
				{{"server_tcp_recv_timeout", "3 * 60 * 1000"}},
				{{"server_tcp_send_timeout", "3 * 1000"}},
				{{"server_udp_recv_timeout", "3 * 60 * 1000"}},
				{{"server_udp_send_timeout", "3 * 1000"}}
	}, "this configvar defined server link argument");

static uint64_t server_tcp_recv_timeout = ~0x0ull;
static uint64_t server_tcp_send_timeout = ~0x0ull;
static uint64_t server_udp_recv_timeout = ~0x0ull;
static uint64_t server_udp_send_timeout = ~0x0ull;

struct ServerConfig_Initer {
	using Type = Vector<Map<std::string, std::string> >;
	
	ServerConfig_Initer() {
		auto& variable = Server_Config_Var->getVariable();

#define XX(name)	\
	if(it2.first == #name) { name = StringCalculate(it2.second); continue; }	

		for(auto& it : variable) {
			for(auto& it2 : it) {
				XX(server_tcp_recv_timeout)
                XX(server_tcp_send_timeout)
                XX(server_udp_recv_timeout)
                XX(server_udp_send_timeout)
			}
		}

		Server_Config_Var->addMonitor("server_01", [](const Type& oldVar, const Type& newVar) {
			for(auto& it : newVar) {
				for(auto& it2 : it) {
					XX(server_tcp_recv_timeout)
            	    XX(server_tcp_send_timeout)
            	    XX(server_udp_recv_timeout)
            	    XX(server_udp_send_timeout)
				}
			}
		});
#undef XX
	}
};

static ServerConfig_Initer ServerConfig_Initer_CXX_01;

}

constexpr const char* g_logger = "server";

Server::Server(const std::string& name, uint32_t thread_num, IOFramework framework)
	: m_isStop(true), m_name(name)
	, m_IOFramework(framework)
	, m_TCPListeners{}
	, m_UDPListeners{}
	, m_IOBase{}
	, m_sigEvents{} {

	switch(m_IOFramework) {
		case SELECT : {
			break;
		}
		case POLL : {
			break;
		}
		case EPOLL : {
			m_IOBase = std::make_shared<EpollIO>(m_name, thread_num);
			break;
		}
		default :
			throw std::invalid_argument("framework invalid");
	}
	m_isStop = false;
}

Server::~Server() {
	if(!m_isStop) {
		loopExit();
	}
}

void Server::setWorkDir(const std::string& path) {
	int32_t rt = chdir(path.c_str());
	if(rt < 0) {
		WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET(g_logger))
			<< "Server " << m_name << " setWorkDir error "
			<< "what : " << strerror(errno);
		::abort();
	}
}

void Server::registerSigEvent(std::function<void()> func, int signum) {
	m_sigEvents.insert(std::make_pair(signum, func));
}

bool Server::bindTCPAddr(Address::ptr addr) {
	if(!addr) {
		return false;
	}

	Domain domain = addr->getDomain();
	Socket::ptr listener;
	if(domain == Domain::IPv4) {
		listener = Socket::CreateIPv4TCPSocket();
	} else if(domain == Domain::IPv6) {
		listener = Socket::CreateIPv6TCPSocket();
	} else {
		return false;
	}
	if(listener->setAddrReuse()
			|| listener->bind(addr) < 0
			|| listener->listen(1024) < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "addr " << addr->visual() << " TCP bind error : "
			<< listener->getErrorMsg();
		return false;
	}
		
	SockFdInfo::ptr sockfd_info =
			SockFdInfoMgr::GetInstance()->addSockFdInfo(listener->getSockfd());
	if(!sockfd_info) {
		return false;
	}

	m_TCPListeners.push_back(listener);
	return true;
}

bool Server::bindUDPAddr(Address::ptr addr) {
	if(!addr) {
		return false;
	}
	Domain domain = addr->getDomain();
	Socket::ptr listener;
	if(domain == Domain::IPv4) {
		listener = Socket::CreateIPv4UDPSocket();
	} else if(domain == Domain::IPv6) {
		listener = Socket::CreateIPv6UDPSocket();
	} else {
		return false;
	}
	if(listener->setAddrReuse()
			|| listener->bind(addr) < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "addr " << addr->visual() << " UDP bind error : "
			<< listener->getErrorMsg();
		return false;
	}
	
	SockFdInfo::ptr sockfd_info =
			SockFdInfoMgr::GetInstance()->addSockFdInfo(listener->getSockfd());
	if(!sockfd_info) {
		return false;
	}
	sockfd_info->setRecvTimeout(server_udp_recv_timeout);
	sockfd_info->setSendTimeout(server_udp_send_timeout);

	m_UDPListeners.push_back(listener);
	return true;
}

bool Server::bindTCPAddrs(const std::vector<Address::ptr>& addrs
	 	, std::vector<Address::ptr>& fails) {
	bool rt = true;
	for(auto& it : addrs) {
		rt = bindTCPAddr(it);
		if(!rt) {
			fails.push_back(it);
			rt = false;
		}
	}
	return rt;
}

bool Server::bindUDPAddrs(const std::vector<Address::ptr>& addrs
	 	, std::vector<Address::ptr>& fails) {
	bool rt = true;
	for(auto& it : addrs) {
		rt = bindUDPAddr(it);
		if(!rt) {
			fails.push_back(it);
			rt = false;
		}
	}
	return rt;
}

void Server::dispatch() {
	m_isStop = false;

	m_IOBase->start();

	for(auto& it : m_TCPListeners) {
		m_IOBase->addEvent(it->getSockfd(), IOBase::READ
				, std::bind(&Server::acceptTCPClient, this, it));
	}
	for(auto& it : m_UDPListeners) {
		m_IOBase->addEvent(it->getSockfd(), IOBase::READ
				, std::bind(&Server::waitUDPClient, this, it));
	}

	while(!m_isStop) {
		sigset_t sigset;
		sigemptyset(&sigset);
		for(auto& it : m_sigEvents) {
			sigaddset(&sigset, it.first);
		}
		pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
		
		struct timespec tv{ .tv_sec = 2, .tv_nsec = 0};

		int signum = sigtimedwait(&sigset, nullptr, &tv);
		if(signum < 0 && errno == EAGAIN) { continue; }
		if(signum < 0 && errno != EAGAIN) {
			WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET(g_logger))
				<< "Server " << m_name << " dispatch sigtimedwait error"
				<< strerror(errno);
			break;
		}
		
		auto it_pair = m_sigEvents.equal_range(signum);
		for(auto it = it_pair.first; it != it_pair.second; ++it) {
			m_IOBase->schedule(it->second);
		}
		m_sigEvents.erase(it_pair.first, it_pair.second);
	}
	
	m_IOBase->stop();
	m_TCPListeners.clear();
	m_UDPListeners.clear();
}

void Server::loopExit() {
	m_isStop = true;
}

void Server::acceptTCPClient(Socket::ptr listener) {
	if(!listener) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Server " << m_name << " warning : "
			<< "TCP listener is nullptr";
		return;
	}

	IPAddress::ptr localAddr =
			std::dynamic_pointer_cast<IPAddress>(listener->getLocalAddress());
	if(!localAddr) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Server " << m_name << " warning : "
			<< "TCP dynamic_pointer_cast error";
		return;
	}
	
	while(!m_isStop) {
		Socket::ptr client = listener->accept();
		if(!client) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "Server " << m_name << " warning : "
				<< "TCP listener accept error  addr : "
				<< localAddr->visual() << ":" 
				<< localAddr->getPort();
			continue;
		}
		
		client->setRecvTimeout(server_tcp_recv_timeout);
		client->setSendTimeout(server_tcp_send_timeout);

		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
				<< "Server " << m_name << " client : "
				<< client->getRemoteAddress()->visual()
				<< " connected";
		if(!m_isStop) {
			m_IOBase->schedule(std::bind(&Server::doInteracteTCPClient, this, client));
		}
	}
}

void Server::waitUDPClient(Socket::ptr listener) {
	if(m_isStop) {
		return;
	}
	if(!listener) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Server " << m_name << " warning : "
			<< "UDP listener is nullptr";
		return;
	}

	IPAddress::ptr localAddr =
			std::dynamic_pointer_cast<IPAddress>(listener->getLocalAddress());
	if(!localAddr) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Server " << m_name << " warning : "
			<< "UDP dynamic_pointer_cast error";
		return;
	}
	if(!m_isStop) {
		doInteracteUDPClient(listener);
		
		m_IOBase->addEvent(listener->getSockfd(), IOBase::READ
				, std::bind(&Server::waitUDPClient, this, listener));
	}
}

}	// namespace webserver
