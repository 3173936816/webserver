#include <iostream>
#include "webserver.h"
#include <thread>
#include <unistd.h>
#include <vector>

webserver::Mutex mutex{webserver::Mutex::RECURSIVE};
webserver::RWMutex rwmutex;
webserver::SpinMutex spinmutex;
webserver::Semaphore sem{1};

int count = 0;
void thread_func() {
	webserver::SpinMutex::Lock locker(spinmutex, webserver::TryLock);

	if(locker.getOwns()) {	
		for(size_t i = 0; i < 1000000; ++i) {
			//webserver::Mutex::Lock locker(mutex, webserver::KeepLock);
			//webserver::RWMutex::WRLock locker(rwmutex, webserver::KeepLock);
			//webserver::SpinMutex::Lock locker(spinmutex);
			//sem.wait();

			int tmp = count;
			++tmp;
			count = tmp;
			sleep(0.001);

			//sem.notify();
		}
	}
}

void test_mutex() {
	std::vector<std::thread> vec;
	for(size_t i = 0; i < 5; ++i) {
		vec.emplace_back(thread_func);		
		sleep(1);
	}

	for(size_t i = 0; i < vec.size(); ++i) {
		vec[i].join();
	}
	std::clog << "count = " << count << std::endl;
}


int main() {
	test_mutex();

	return 0;
}
