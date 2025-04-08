#include "webserver.h"
#include "async.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

using namespace webserver;

void sleep_func(int i) {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "start: " << i;

	async::Sleep(2);
	//sleep(2);
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "end: " << i;
}

void test_sleep() {
	EpollIO::ptr epio = std::make_shared<EpollIO>("ep", 1);
	
	epio->schedule(std::bind(sleep_func, 0));
	epio->schedule(std::bind(sleep_func, 1));
}

void test_async_connect() {
	int sockfd = async::Socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		std::cout << "socket error : " << strerror(errno) << std::endl;
		return;
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(9920);
	inet_pton(AF_INET, "192.168.21.1", &server.sin_addr.s_addr);
	
	int rt = async::Connect_with_timeout(sockfd
			, (struct sockaddr*)&server, sizeof(server), 5000);
	if(rt < 0) {
		std::cout << "connect error : " << strerror(errno) << std::endl;
		return;
	}
	
	std::string msg_send = "GET / HTTP/1.1\r\n\r\n";
	rt = async::Send(sockfd, msg_send.c_str(), msg_send.size() + 1, 0);
	if(rt < 0) {
		std::cout << "send error : " << strerror(errno) << std::endl;
		return;
	}
	
	std::string msg_recv;
	msg_recv.resize(1024);
	rt = async::Recv(sockfd, &msg_recv[0], msg_recv.size(), 0);
	if(rt < 0) {
		std::cout << "read error : " << strerror(errno) << std::endl;
		return;
	}
	
	async::Close(sockfd);
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "get msg : \n" << msg_recv;
}

void test_async_accept() {
	int sockfd = async::Socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		std::cout << "socket error : " << strerror(errno) << std::endl;
		return;
	}
	
	int i = 1;
	socklen_t len2 = sizeof(int);
	async::Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, len2);
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(9920);
	inet_pton(AF_INET, "192.168.21.144", &server.sin_addr.s_addr);
	
	async::Bind(sockfd, (struct sockaddr*)&server, sizeof(server));
	async::Listen(sockfd, 1024);
	
	struct sockaddr_in client;
	memset(&client, 0x00, sizeof(client));
	socklen_t len = sizeof(client);
	int client_sock = async::Accept(sockfd, (struct sockaddr*)&client, &len);
	if(client_sock < 0) {
		std::cout << "accept error : " << strerror(errno) << std::endl;
		return;
	}
	char buff[16] = {0};
	inet_ntop(AF_INET, &client.sin_addr.s_addr, buff, sizeof(buff));
	std::cout << "host : " << buff << " is connected" << std::endl;
	
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	async::Setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	std::string msg_send = "hello! welcome to my server";
	int rt = async::Send(client_sock, msg_send.c_str(), msg_send.size() + 1, 0);
	if(rt < 0) {
		std::cout << "send error : " << strerror(errno) << std::endl;
		return;
	}
	
	std::string msg_recv;
	msg_recv.resize(1024);
	rt = async::Recv(client_sock, &msg_recv[0], msg_recv.size(), 0);
	if(rt < 0) {
		std::cout << "read error : " << strerror(errno) << std::endl;
		return;
	}
	
	async::Close(sockfd);
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())
		<< "get msg : \n" << msg_recv;
}

int main() {
	//test_sleep();
	//test_async_connect();
	//test_async_accept();
	
	 EpollIO::ptr ep = std::make_shared<EpollIO>("ep", 1);
	 //ep->schedule(test_async_connect);
	 //ep->schedule(std::bind(sleep_func, 0));
	 ep->schedule(test_async_accept);

	return 0;
}
