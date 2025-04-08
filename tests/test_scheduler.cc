#include "webserver.h"

using namespace webserver;

class SchedulerExample : public Scheduler {
public:
	using ptr = std::shared_ptr<SchedulerExample>;
	SchedulerExample(const std::string& name, uint32_t num)
		: Scheduler(name, num) {
	}
	
	void wait() override {}
	void remind() override {}
};


void func(size_t i) {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "1  " << i;
	now_scheduler::ReSchedule("sc_th_2");
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "hhhhh" << i;
}

void test_scheduler() {
	SchedulerExample::ptr sc(new SchedulerExample("sc", 3));
	sc->start();
	
	for(size_t i = 0; i < 5; ++i) {
		sc->schedule("sc_th_0", std::bind(func, i));
	}
	sc->stop();
}

int main() {
	signal(SIGSEGV, SIG_IGN);
	test_scheduler();

	return 0;
}
