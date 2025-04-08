#include "thread.h"
#include "log.h"
#include "exception.h"
#include "macro.h"

#include <string.h>
#include <boost/thread.hpp>

namespace webserver {

static constexpr const char* s_logger = "system";

Thread::Thread(const std::string& name, std::function<void()> func)
	: m_tid(0), m_id(0), m_joinable(false)
	, m_name(name), m_sem{}, m_func(func) {

	int32_t rt = pthread_create(&m_id, nullptr, &Thread::start_routine, this);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger)) << "Thread::Thread"
			<< " error --- name : " << name << " errmeg : " << strerror(rt);
		throw Thread_error("Thread::Thread --- pthread_create error");
	}
	m_joinable = true;
	m_sem.wait();
}

void Thread::detach() {
	if(!joinable()) {
		throw Thread_error("Thread::detach --- invaild argument");
	}	

	int32_t rt = pthread_detach(m_id);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger)) << "Thread::deatch"
			<< " error --- name : " << m_name << "  Tid : " << m_tid
			<< "  errmeg : " << strerror(rt);
		throw Thread_error("Thread::detach --- pthread_deatch error");
	}
	invalid();
}

void Thread::join() {
	if(!joinable()) {
		throw Thread_error("Thread::join --- invaild argument");
	}	

	int32_t rt = pthread_join(m_id, nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger)) << "Thread::join"
			<< " error --- name : " << m_name << "  Tid : " << m_tid
			<< "  errmeg : " << strerror(rt);
		throw Thread_error("Thread::join --- pthread_join error");
	}
	invalid();
}

uint32_t Thread::Hardware_concurrency() {
	return boost::thread::hardware_concurrency();
}

void* Thread::start_routine(void *arg) {
	Thread* th = static_cast<Thread*>(arg);
	th->m_tid = GetThreadTid();
	int32_t rt = pthread_setname_np(th->m_id, th->m_name.substr(0, 15).c_str());
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger)) << "Thread::start_routine"
			<< " error --- name : " << th->m_name << "  Tid : " << th->m_tid
			<< "  errmeg : " << strerror(rt);
		throw Thread_error("Thread::start_rountine --- pthread_setname_np error");
	}
	
	th->m_sem.notify();
	
	std::function<void()> func;
	func.swap(th->m_func);
	
	try {
		func();
	} catch(std::exception& e) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger)) 
			<< "Thread::start_routine -- func throw a exception"
			<< "  exception : " << e.what();
	}
	
	return nullptr;
}

void Thread::invalid() {
	m_tid = 0;
	m_id = 0;
	m_joinable = false;
	m_name = "";
}

uint32_t now_thread::GetTid() {
	return GetThreadTid();
}

pthread_t now_thread::GetId() {
	return pthread_self();
}

std::string now_thread::GetThreadName() {
	char buff[17] = {0};
	int32_t rt = pthread_getname_np(GetId(), buff, sizeof(buff));
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "now_thread::GetThreadName --- pthread_getname_np error");
		return "ERRORNAME";
	}
	return std::string(buff);
}

bool now_thread::Yield() {
	return pthread_yield() == 0;
}

void now_thread::Sleep_for(uint32_t second) {
	sleep(second);
}

}	// namespace webserver
