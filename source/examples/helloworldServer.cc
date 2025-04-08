#include "webserver.h"

class HelloWorldServer : public webserver::Server {
public:
	using ptr = std::shared_ptr<HelloWorldServer>;
	
	HelloWorldServer(const std::string& name, uint32_t thread_num);
	~HelloWorldServer();
	
	webserver::http::HttpResponse::ptr getHelloWorldHtml_http();

protected:
	void doInteracteTCPClient(webserver::Socket::ptr client) override;
	void doInteracteUDPClient(webserver::Socket::ptr client) override;
};

HelloWorldServer::HelloWorldServer(const std::string& name, uint32_t thread_num)
	: Server(name, thread_num) {
}

HelloWorldServer::~HelloWorldServer() = default;

webserver::http::HttpResponse::ptr HelloWorldServer::getHelloWorldHtml_http() {
	std::ifstream ifs("hello_world.html");
	std::string context;
	std::string tmp;
	while(ifs >> tmp) {
		context += tmp;
	}
	ifs.close();

	webserver::http::HttpResponse::ptr response
			= std::make_shared<webserver::http::HttpResponse>(
				webserver::http::HttpStatus::OK);
	response->addHeaderField("Content-Type", "text/html");
	response->addHeaderField("Content-Length", std::to_string(context.size()));
	response->setBody(context);
	
	return response;
}

void HelloWorldServer::doInteracteTCPClient(webserver::Socket::ptr client) {
	webserver::http::HttpResponse::ptr response = getHelloWorldHtml_http();
	std::string response_str = response->getResponse();

	while(true) {
		char buff[1024] = {0};
		ssize_t rt = client->recv(buff, sizeof(buff));
		if(rt > 0) {
			WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
				<< "Server TCP recv : "
				<< buff;

			client->send(response_str.c_str(), response_str.size());
		} else if(rt == 0) {
			break;
		} else {
			WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
				<< "Server TCP recv error  what: " << strerror(errno);
			break;
		}
	}

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "TCP " << client->getRemoteAddress()->visual()
		<< " disconnect";
}

void HelloWorldServer::doInteracteUDPClient(webserver::Socket::ptr client) {
	webserver::http::HttpResponse::ptr response = getHelloWorldHtml_http();
	std::string response_str = response->getResponse();

	char buff[1024] = {0};
	webserver::Address::ptr src_addr;
	ssize_t rt = client->recvfrom(buff, sizeof(buff), src_addr);
	if(rt > 0) {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
			<< "Server TCP recv : "
			<< buff;

		client->sendto(response_str.c_str(), response_str.size(), src_addr);
	} else {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
			<< "Server TCP recv error  what: " << strerror(errno);
	}
}

int main() {
	// create server
	HelloWorldServer::ptr server = std::make_shared<HelloWorldServer>("lcy/1.0", 5);
	
	// bind address
	webserver::Address::ptr tcp_addr
			 = std::make_shared<webserver::IPv4Address>("192.168.21.144", 9920);
	webserver::Address::ptr udp_addr
			 = std::make_shared<webserver::IPv4Address>("192.168.21.144", 9921);
	if(!server->bindTCPAddr(tcp_addr) || !server->bindUDPAddr(udp_addr)) {
		std::cout << "bind address error" << std::endl;
		return -1;
	}

	// change work dir
	server->setWorkDir("/home/lcy/projectsMySelf/MyWebserver/tests/html");
	
	// set exit sigevent
	server->registerSigEvent([server](){ 
		std::cout << "Server recv signal to exit, please wait for a while" << std::endl;
		server->loopExit(); 
	}, SIGINT);

	server->registerSigEvent([server](){ 
		std::cout << "crtl + \\" << std::endl;
	 }, SIGQUIT);
	
	// start event loop
	server->dispatch();

	return 0;
}
