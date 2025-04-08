#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "webserver.h"

using namespace webserver;

void func(size_t i) {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "start" << '\t' << i;
	now_scheduler::ReSchedule("ep_th_1");
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "end" << '\t' << i;
}

void test_sc() {
	EpollIO::ptr epio(new EpollIO("ep", 5));
	epio->start();
	
	for(size_t i = 0; i < 300; ++i) {
		epio->schedule("ep_th_2", std::bind(func, i));
	}
	epio->stop();
}

void test_epio() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0) {
		std::cout << "sock_fd error" << std::endl;
		return;
	}

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "sock_fd : " << sock_fd;
	EpollIO::ptr epio(new EpollIO("ep", 5));
	
	epio->start();

	epio->addEvent(sock_fd, EpollIO::READ, [=]() {
		int flag = fcntl(sock_fd, F_GETFL, 0);
		fcntl(sock_fd, F_SETFL, flag | O_NONBLOCK);

		char buff[2048] = {0};
		while(read(sock_fd, buff, sizeof(buff)) > 0) {
			WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << buff;
		}
		close(sock_fd);
	});
	
	epio->addEvent(sock_fd, EpollIO::WRITE, [=]() {
		std::string http_con = "GET / HTTP/1.1\r\n\r\n";

		int rt = write(sock_fd, http_con.c_str(), http_con.size() + 1);
		if(rt < 0) {
			WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "write Error  " << strerror(errno);
			return;
		}
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "Write is OK";
	});

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "eventCout : " << epio->eventCount();
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	inet_pton(AF_INET, "36.152.44.93", &server_addr.sin_addr.s_addr);
	
	std::cout << "addr int : " << server_addr.sin_addr.s_addr << std::endl;
		
    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(strerror(errno));
        return;
    }
	
	epio->stop();
}

int main() {
	test_sc();
	//signal(SIGPIPE, SIG_IGN);
	//test_epio();

	return 0;
}
