#include "webserver.h"

using namespace webserver;

void test_socket_client_TCP() {
	Socket::ptr sock = std::make_shared<Socket>(Domain::IPv4, SockType::TCP);
	Address::ptr addr = IPv4Address::ResoluteAnyTCP("www.baidu.com", "80");
	//Address::ptr addr = std::make_shared<IPv4Address>("192.168.21.1", 9920);
	
	sock->setRecvTimeout(5000);
	sock->setNonBlock();

	int rt = sock->connect_with_timeout(addr, 5000);
	if(rt < 0) {
		std::cout << "connect error : " << strerror(errno) << std::endl;
		return;
	}
	
	rt = sock->send("GET / HTTP/1.1\r\n\r\n", sizeof("GET / HTTP/1.1\r\n\r\n"));	
	if(rt < 0) {
		std::cout << "send error : " << strerror(errno) << std::endl;
		return;
	}
	
	std::string recv_str;
	recv_str.resize(1024);
	rt = sock->recv(&recv_str[0], recv_str.size());
	if(rt < 0) {
		std::cout << "recv error : " << strerror(errno) << std::endl;
		return;
	}
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())	<< recv_str;
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << sock->getRecvTimeout();	
}

void test_socket_server_TCP() {
	Socket::ptr sock = std::make_shared<Socket>(Domain::IPv6, SockType::TCP);
	sock->setAddrReuse();
	Address::ptr addr = std::make_shared<IPv6Address>("fe80::4b57:85a2:3636:3809%ens33", 9920);
	
	sock->bind(addr);
	sock->listen(1024);
	
	Address::ptr client_addr;	
	Socket::ptr client_sock = sock->accept(client_addr);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
			 << client_addr->visual() << "  connected";

	std::string send_str = "welcome to my webserver\n";
	client_sock->send(send_str.c_str(), send_str.size() + 1);
	
	std::string recv_str;
	recv_str.resize(1024);
	client_sock->recv(&recv_str[0], recv_str.size());
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << recv_str;
}


int main() {
	//test_socket_client_TCP();
	test_socket_server_TCP();

	//EpollIO::ptr epio = std::make_shared<EpollIO>("ep", 1);
	//epio->start();
	//epio->schedule(test_socket_server_TCP);
	//epio->stop();

	return 0;
}
