#include "webserver.h"

std::string getContent(const std::string& file) {
	std::ifstream ifs(file);
	std::string context;
	std::string tmp;
	while(std::getline(ifs, tmp)) {
		context += tmp;
	}
	ifs.close();
	
	return context;
}

void test_httpServer() {
	webserver::http::HttpServer::ptr server 
			= std::make_shared<webserver::http::HttpServer>("lcy/1.0.0", 5);
	
	// bind address
	webserver::Address::ptr tcp_addr
			 = std::make_shared<webserver::IPv4Address>("192.168.21.144", 9920);
	webserver::Address::ptr udp_addr
			 = std::make_shared<webserver::IPv4Address>("192.168.21.144", 9921);
	if(!server->bindTCPAddr(tcp_addr) || !server->bindUDPAddr(udp_addr)) {
		std::cout << "bind address error" << std::endl;
		return;
	}
	
	// set servlet
	webserver::http::NotFoundServlet::ptr defaultServlet = 
			std::make_shared<webserver::http::NotFoundServlet>(server->getServerName());
	server->getServletDispatch()->setDefaultServlet(defaultServlet);
	
	// set index
	server->getServletDispatch()->addExactServlet("/", 
			[](webserver::http::HttpSession::ptr session
			, webserver::http::HttpRequest::ptr request
			, webserver::http::HttpResponse::ptr response) {

		std::string content = getContent("index.html");

		response->addHeaderField("Content-Type", "text/html");
		response->addHeaderField("Content-Length", std::to_string(content.size()));
		response->setBody(content);
	});
	
	// set other page
	server->getServletDispatch()->addGlobServlet("/*.html", 
			[](webserver::http::HttpSession::ptr session
			, webserver::http::HttpRequest::ptr request
			, webserver::http::HttpResponse::ptr response) {

		std::string content = getContent(request->getPath().substr(1));

		response->addHeaderField("Content-Type", "text/html");
		response->addHeaderField("Content-Length", std::to_string(content.size()));
		response->setBody(content);
	});
	
	// change work dir
	server->setWorkDir("/home/lcy/projectsMySelf/MyWebserver/tests/html");
	
	// set exit sigevent
	server->registerSigEvent([server](){ 
		std::cout << "Server recv signal to exit, please wait for a while" << std::endl;
		server->loopExit(); 
	}, SIGINT);

	// start event loop
	server->dispatch();
}

int main() {
	test_httpServer();

	return 0;
}
