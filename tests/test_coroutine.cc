#include "webserver.h"
#include <set>

using namespace webserver;

void test_UUid() {
	std::set<uint64_t> se;
	for(size_t i = 0; i < 10; ++i) {
		sleep(0.001);
		std::cout << GetUUID() << std::endl;
		se.insert(GetUUID());
	}
	std::cout << se.size() << std::endl;
}

void test_backtrace() {
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << PrintStackInfo(10);
	WEBSERVER_ASSERT2(0, "ppppp");
}

void cor_func() {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "cor_func satrt";
	now_coroutine::SwapOut();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "cor_func exec";
	now_coroutine::SwapOut();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "cor_func term";
	//now_coroutine::SwapOutToTerm();
}

void cor_func2() {
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_coroutine::GetCid();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_coroutine::GetState();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "cor_func2 term";
}

void test_coroutine() {
	Config::EffectConfig("../webserver_confs.yml");

	Coroutine::ptr cor(new Coroutine(cor_func));

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "test_coroutine satrt";
	cor->swapIn();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "test_coroutine exec";
	cor->swapIn();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "test_coroutine term";
	cor->swapIn();
	
	cor->reset(cor_func2);
	cor->swapIn();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "test_coroutine term2";
	
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_coroutine::GetCid();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << now_coroutine::GetState();
}

class TT {
public:
	~TT() { std::cout << "~TT is Ok" << std::endl; }
};
	
void tt() {
	std::shared_ptr<TT> t = std::make_shared<TT>();
	//now_coroutine::SwapOutToTerm();
}

void test_stack() {
	Coroutine::ptr cor(new Coroutine(tt));
	cor->swapIn();
	std::cout << "end" << std::endl;
}

void th_co() {
	Thread::ptr th(new Thread("kkk", test_coroutine));
	th->join();
}

void test_reset() {
	Coroutine::ptr cor(new Coroutine(cor_func));
	for(size_t i = 0; i < 10000; ++i) {
		cor->reset(test_stack);
	}
}

int main() {
//	test_UUid();
//	test_backtrace();
//	test_coroutine();
//	test_stack();
	//th_co();	
	test_reset();

	return EXIT_SUCCESS;
}
