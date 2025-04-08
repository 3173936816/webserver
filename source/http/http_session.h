#ifndef __WEBSERVER_HTTP_SESSION_H__
#define __WEBSERVER_HTTP_SESSION_H__

#include <memory>

#include "../socket.h"
#include "http.h"

namespace webserver {
namespace http {

class HttpSession {
public:
	using ptr = std::shared_ptr<HttpSession>;
	
	HttpSession(Socket::ptr client, bool owner = true);
	~HttpSession();
	
	bool recvTCPHttpRequest(HttpRequest::ptr& request);
	bool sendTCPHttpResponse(HttpResponse::ptr response);
	
	bool recvUDPHttpRequest(HttpRequest::ptr& request, Address::ptr& src_addr);
	bool sendUDPHttpResponse(HttpResponse::ptr response, Address::ptr dest_addr);
	
	bool isOwner() const { return m_owner; }
	Socket::ptr getClient() const { return m_client; }
	
	Socket::ptr release();
	bool close();

private:
	bool m_owner;
	Socket::ptr m_client;
};

}	// namespace http
}	// namespace webserver

#endif // ! __WEBSERVER_HTTP_SESSION_H__
