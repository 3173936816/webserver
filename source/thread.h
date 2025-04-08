#ifndef __WEBSERVER_THREAD_H__
#define __WEBSERVER_THREAD_H__

#include <memory>
#include <string>
#include <pthread.h>
#include <functional>

#include "synchronism.h"
#include "noncopyable.h"

namespace webserver {

class Thread {
public:
	using ptr = std::shared_ptr<Thread>;
	
	Thread(const std::string& name, std::function<void()> func);
	~Thread() = default;

	void detach();
	void join();

	bool joinable() const { return m_joinable; };
	uint32_t getThreadTid() const { return m_tid; };
	std::string getThreadName() const { return m_name; };
	
	static uint32_t Hardware_concurrency(); 

private:
	static void* start_routine(void *arg);
	void invalid();

private:
	pid_t m_tid;
	pthread_t m_id;
	bool m_joinable;
	std::string m_name;
	mutable Semaphore m_sem;
	std::function<void()> m_func;
};

//use as a namespace
class now_thread : Noncopyable {
public:

	static uint32_t GetTid();
	static pthread_t GetId();
	static std::string GetThreadName();
	static bool Yield();
	static void Sleep_for(uint32_t second);

private:
	now_thread() = delete;
	~now_thread() = delete;
};

}	// namespace webserver

#endif // ! __WEBSERVER_THREAD_H__
