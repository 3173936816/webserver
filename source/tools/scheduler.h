#ifndef __WEBSERVER_SCHEDULER_H__
#define __WEBSERVER_SCHEDULER_H__

#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <list>
#include <functional>

#include "util.h"
#include "thread.h"
#include "coroutine.h"
#include "synchronism.h"
#include "noncopyable.h"
#include "../async.h"

#define SCHEDULER_REMIND_THRESHOLD 50

namespace webserver {

class Scheduler {
	friend class now_scheduler;
	friend class async;
public:
	using ptr = std::shared_ptr<Scheduler>;
	using MutexType = Mutex;
	
	enum TimeoutMode {
		DISCARD = 0,
		TRIGGER = 1
	};

	Scheduler(const std::string& name, uint32_t thread_num, 
			TimeoutMode mode = TRIGGER, uint64_t timeout = 5000);
	virtual ~Scheduler();
	
	void start();
	void stop();
	void setTimeoutMode(TimeoutMode mode) { m_mode = mode; }

	void schedule(std::function<void()> func);
	void schedule(Coroutine::ptr co_func);
	void schedule(const std::string& thread_name, std::function<void()> func);
	void schedule(const std::string& thread_name, Coroutine::ptr co_func);
	void schedule(pid_t thread_tid, std::function<void()> func);
	void schedule(pid_t thread_tid, Coroutine::ptr co_func);

	template <typename Iterator>
	void batchCoSchedule(Iterator beg, Iterator end);
	
	template <typename Iterator>
	void batchFunSchedule(Iterator beg, Iterator end);
	
	uint32_t taskCount() const { return m_taskCount.load(); }
	uint32_t getTaskTimeout() const { return m_timeout.load(); }
	TimeoutMode getTimeoutMode() const { return m_mode.load(); }
	uint32_t waitingThreadCount() const { return m_waitingThreadCount.load(); }
	const std::vector<Thread::ptr>& getThreads() const { return m_threads; }
	
	virtual void wait() = 0;
	virtual void remind() = 0;

protected:
	virtual bool condStatisfy();

private:
	void worker();

private:
	void scheduleNoLock(Coroutine::ptr co_func);
	void scheduleNoLock(const std::string& thread_name, Coroutine::ptr co_func);
	void scheduleNoLock(pid_t thread_tid, Coroutine::ptr co_func);

protected:
	enum Task_Sign {
		SIGN_ANY 	= 0,
		SIGN_NAME 	= 1,
		SIGN_TID 	= 2
	};

	//Scheduler task
	struct Sc_task {
		using ptr = std::shared_ptr<Sc_task>;

		Sc_task(Coroutine::ptr co_func);
		Sc_task(const std::string& thread_name, Coroutine::ptr co_func);
		Sc_task(pid_t thread_tid, Coroutine::ptr co_func);
		~Sc_task() = default;
	
		void setTimeout(uint64_t sec) { timeout = sec; }

		Task_Sign sign;
		uint64_t timeout;
		std::string thread_name;	//SIGN_NAME
		pid_t thread_tid;			//SIGN_TID
		Coroutine::ptr co_func;
	};

private:
	thread_local static Scheduler* m_currScheduler;
	thread_local static Sc_task::ptr m_currSc_task;

public:
	static uint64_t ERR_TIMEOUT;

private:
	mutable MutexType m_mtx;
	const std::string m_name;
	std::atomic<bool> m_stop;
	std::atomic<uint64_t> m_timeout;
	std::atomic<TimeoutMode> m_mode;
	std::atomic<uint32_t> m_taskCount;
	std::vector<Thread::ptr> m_threads;
	std::atomic<uint32_t> m_threadCount;
	std::list<Sc_task::ptr> m_coTasks;
	std::atomic<uint32_t> m_waitingThreadCount;
};

// use as a namespace
class now_scheduler : Noncopyable {
public:

	static std::string GetSchedulerName();
	static uint64_t GetSchedulerTimeout();

	static void ReSchedule();
	static void ReSchedule(const std::string& thread_name);
	static void ReSchedule(pid_t thread_tid);

	static void Schedule(Coroutine::ptr co_func);
	static void Schedule(const std::string& thread_name, Coroutine::ptr co_func);
	static void Schedule(pid_t thread_tid, Coroutine::ptr co_func);

	static void Schedule(std::function<void()> func);
	static void Schedule(const std::string& thread_name, std::function<void()> func);
	static void Schedule(pid_t thread_tid, std::function<void()> func);
	
private:
	now_scheduler() = delete;
	~now_scheduler() = delete;
};


//template func
template <typename Iterator>
void Scheduler::batchCoSchedule(Iterator beg, Iterator end) {
	MutexType::Lock locker(m_mtx);
	bool need_remind = m_coTasks.empty();
	for(; beg != end; ++beg) {
		Sc_task::ptr new_task(new Sc_task(*beg));
		new_task->setTimeout(GetMs() + m_timeout);
		m_coTasks.emplace_back(std::move(new_task));
		++m_taskCount;
	}
	if(need_remind) {
		remind();
	}
}

template <typename Iterator>
void Scheduler::batchFunSchedule(Iterator beg, Iterator end) {
	MutexType::Lock locker(m_mtx);
	bool need_remind = m_coTasks.empty();
	for(; beg != end; ++beg) {
		Coroutine::ptr co_func(new Coroutine(*beg)); 
		Sc_task::ptr new_task(new Sc_task(co_func));
		new_task->setTimeout(GetMs() + m_timeout);
		m_coTasks.emplace_back(std::move(new_task));
		++m_taskCount;
	}
	if(need_remind) {
		remind();
	}
}

}	// namespace webserver

#endif // ! __WEBSERVER_SCHEDULER_H__
