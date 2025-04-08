#include "webserver.h"
#include <vector>
#include <thread>

using namespace webserver;

uint32_t m = 0;

Mutex mtx;

void func() {
	//for(size_t i = 0; i < 100000; ++i) {
	//	Mutex::Lock locker(mtx);
	//	uint32_t tmp = m;
	//	++tmp;
	//	m = tmp;
	//	sleep(0.02);
	//}
	//sleep(50);
	
	Mutex::Lock locker(mtx);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "++++++++++++++++++++++++++++++++++++++++++";
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_thread::GetTid();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_thread::GetId() << "   " << std::this_thread::get_id();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_thread::GetThreadName();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "++++++++++++++++++++++++++++++++++++++++++" << std::endl;
}

void test_thread() {
	std::vector<Thread::ptr> vec;
	for(size_t i = 0; i < 5; ++i) {
		vec.emplace_back(new Thread("thread_" + std::to_string(i), func));
		//vec[i]->detach();
		//std::cout << vec[i]->getThreadTid() << std::endl;
	}
	
	for(size_t i = 0; i < 5; ++i) {
		vec[i]->join();
	}
}

int main() {
	test_thread();
	//std::cout << m << std::endl;
	std::cout << Thread::Hardware_concurrency() << std::endl;

	return 0;
}
