#ifndef __WEBSERVER_HTTP_SERVER_H__
#define __WEBSERVER_HTTP_SERVER_H__

#include <memory>

#include "http.h"
#include "http_session.h"
#include "http_servlet.h"
#include "../server.h"

namespace webserver {
namespace http {

class HttpServer : public Server {
public:
	using ptr = std::shared_ptr<HttpServer>;
	
	HttpServer(const std::string& name, uint32_t thread_num, IOFramework framework = EPOLL);
	~HttpServer();
	
	ServletDispatch::ptr getServletDispatch() const { return m_servletDispatch; }

protected:
	void doInteracteTCPClient(Socket::ptr client) override;
	void doInteracteUDPClient(Socket::ptr client) override;

private:
	ServletDispatch::ptr m_servletDispatch;
};

}
}	// namespace webserver

#endif // ! __WEBSERVER_HTTP_SERVER_H__
