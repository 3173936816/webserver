#include "webserver.h"

using namespace webserver;

void test_tcpServer() {
//	TcpServer::ptr server = std::make_shared<TcpServer>("tcp_server");
//	IPv4Address::ptr ipv4 = std::make_shared<IPv4Address>("192.168.21.144", 9920);
//	server->bindServerAddr(ipv4);
//	
//	server->start(5);
//	
//	sleep(5);
//	server->stop();
}

int main() {
	test_tcpServer();

	return 0;
}
