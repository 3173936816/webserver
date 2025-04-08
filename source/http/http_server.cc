#include "http_server.h"
#include "log.h"

namespace webserver {
namespace http {

static constexpr const char* g_logger = "system";

HttpServer::HttpServer(const std::string& name
		, uint32_t thread_num, IOFramework framework)
	: Server(name, thread_num, framework), m_servletDispatch{} {

	m_servletDispatch = std::make_shared<ServletDispatch>();
}
 
HttpServer::~HttpServer() = default;

void HttpServer::doInteracteTCPClient(Socket::ptr client) {
	HttpSession::ptr session = std::make_shared<HttpSession>(client);

	while(true) {
		HttpRequest::ptr req = std::make_shared<HttpRequest>();
		bool rt = session->recvTCPHttpRequest(req);
		if(!rt || req->findHeaderField("Connection", "keep-alive") == "close") {
			break;
		}
		
		HttpResponse::ptr rep = std::make_shared<HttpResponse>();
		m_servletDispatch->handle(session, req, rep);
		
		session->sendTCPHttpResponse(rep);
	}
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "TCP " << client->getRemoteAddress()->visual()
		<< " disconnect";
}

void HttpServer::doInteracteUDPClient(Socket::ptr client) {
	HttpSession::ptr session = std::make_shared<HttpSession>(client, false);
	
	Address::ptr addr;
	HttpRequest::ptr req = std::make_shared<HttpRequest>();
	bool rt = session->recvUDPHttpRequest(req, addr);
	if(rt) {
		HttpResponse::ptr rep = std::make_shared<HttpResponse>();
		m_servletDispatch->handle(session, req, rep);
		
		session->sendUDPHttpResponse(rep, addr);
	}
}

} // namespace http
} // namespace webserver
