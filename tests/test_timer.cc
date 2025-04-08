#include "webserver.h"

using namespace webserver;

void test_timer() {
	EpollIO::ptr epio = std::make_shared<EpollIO>("ep", 5);
	epio->start();
	
	Timer::ptr timer1 = epio->addTimer(2000, []() {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) 
			<< "this is other timer  interval 2000ms";
	});
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "add timer";
	Timer::ptr timer = epio->addTimer(2000, []() {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) 
			<< "this is a timer  interval 2000ms";
	}, true);

	
	//sleep(1);
	//timer1->refresh();
	
	sleep(13);
	timer->cancel();

	epio->stop();
}


void test_timer2() {
	EpollIO::ptr epio = std::make_shared<EpollIO>("ep", 5);
	epio->start();

	Timer::ptr timer1 = epio->addTimer(60 * 1000, []() {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) 
			<< "this is other timer  interval 60";
	});
	
	Timer::ptr timer2 = epio->addTimer(4 * 1000, []() {
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) 
			<< "this is other timer  interval 60";
	});
	
	timer2->cancel();
	
	sleep(6);

	epio->stop();
}

int main() {
	//test_timer();
	test_timer2();

	return 0;
}
