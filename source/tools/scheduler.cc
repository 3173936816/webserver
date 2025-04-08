#include "scheduler.h"
#include "log.h"
#include "util.h"
#include "macro.h"

namespace webserver {

constexpr const char* g_logger = "system";

thread_local Scheduler* Scheduler::m_currScheduler{nullptr};
thread_local Scheduler::Sc_task::ptr Scheduler::m_currSc_task{nullptr};

uint64_t Scheduler::ERR_TIMEOUT = ~0x0ull;

// Scheduler::Sc_task
Scheduler::Sc_task::Sc_task(Coroutine::ptr co_func)
	: sign(SIGN_ANY), timeout(0), thread_name{}
	, thread_tid(0), co_func(co_func) {
}

Scheduler::Sc_task::Sc_task(const std::string& thread_name, Coroutine::ptr co_func)
	: sign(SIGN_NAME), timeout(0), thread_name(thread_name)
	, thread_tid(0), co_func(co_func) {
}

Scheduler::Sc_task::Sc_task(pid_t thread_tid, Coroutine::ptr co_func)
	: sign(SIGN_TID), timeout(0), thread_name{}
	, thread_tid(thread_tid), co_func(co_func) {
}

// Scheduler
Scheduler::Scheduler(const std::string& name, uint32_t thread_num, 
			TimeoutMode mode, uint64_t timeout)
	: m_mtx{}, m_name(name.substr(0, 10))
	, m_stop(true), m_timeout(timeout)
	, m_mode(mode), m_taskCount(0), m_threads{}
	, m_threadCount(thread_num), m_coTasks{}
	, m_waitingThreadCount(0) {
}

void Scheduler::start() {
	if(!m_stop) {
		return;
	}
	m_stop = false;

	for(uint32_t i = 0; i < m_threadCount; ++i) {
		std::string th_name = m_name + "_th_" + std::to_string(i);
		Thread::ptr th(new Thread(th_name, std::bind(&Scheduler::worker, this)));
		m_threads.emplace_back(std::move(th));
	}
}

void Scheduler::stop() {
	if(m_stop) {
		return;
	}
	m_stop = true;	
	
	for(size_t i = 0; i < m_threads.size(); ++i) {
		m_threads[i]->join();
	}
}

Scheduler::~Scheduler() = default;

bool Scheduler::condStatisfy() {
	return !m_stop || m_taskCount;
}

void Scheduler::scheduleNoLock(Coroutine::ptr co_func) {
	bool need_remind = m_coTasks.empty();
	Sc_task::ptr new_task(new Sc_task(co_func));
	new_task->setTimeout(GetMs() + m_timeout);
	m_coTasks.emplace_back(std::move(new_task));
	++m_taskCount;
	
	if(need_remind) {
		remind();
	}
}

void Scheduler::scheduleNoLock(const std::string& thread_name, Coroutine::ptr co_func) {
	bool need_remind = m_coTasks.empty();
	Sc_task::ptr new_task(new Sc_task(thread_name, co_func));
	new_task->setTimeout(GetMs() + m_timeout);
	m_coTasks.emplace_back(std::move(new_task));
	++m_taskCount;
	
	if(need_remind) {
		remind();
	}
}
void Scheduler::scheduleNoLock(pid_t thread_tid, Coroutine::ptr co_func) {
	bool need_remind = m_coTasks.empty();
	Sc_task::ptr new_task(new Sc_task(thread_tid, co_func));
	new_task->setTimeout(GetMs() + m_timeout);
	m_coTasks.emplace_back(std::move(new_task));
	++m_taskCount;
	
	if(need_remind) {
		remind();
	}
}

void Scheduler::schedule(std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(co_func);
}

void Scheduler::schedule(Coroutine::ptr co_func) {
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(co_func);
}

void Scheduler::schedule(const std::string& thread_name, std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(thread_name, co_func);
}

void Scheduler::schedule(const std::string& thread_name, Coroutine::ptr co_func) {
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(thread_name, co_func);
}

void Scheduler::schedule(pid_t thread_tid, std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(thread_tid, co_func);
}

void Scheduler::schedule(pid_t thread_tid, Coroutine::ptr co_func) {
	MutexType::Lock locker(m_mtx);
	scheduleNoLock(thread_tid, co_func);
}

void Scheduler::worker() {
	m_currScheduler = this;
	async::setAsync(true);

	std::string curr_thread_name = now_thread::GetThreadName();
	pid_t curr_thread_tid = now_thread::GetTid();
	
	Coroutine::ptr wait_coFunc(new Coroutine(std::bind(&Scheduler::wait, this)));

	while(condStatisfy()) {
		MutexType::Lock locker(m_mtx);

		if(!m_coTasks.empty()) {
			uint64_t now_time = 0;
			
			if(m_coTasks.size() >= 
				(m_threadCount - m_waitingThreadCount) * 
					SCHEDULER_REMIND_THRESHOLD) {
				remind();
			}

			for(auto it = m_coTasks.begin(); it != m_coTasks.end(); ++it) {
				Sc_task* task = it->get();
				now_time = GetMs();
				
				if(task->timeout < now_time
					|| task->sign == SIGN_ANY
					|| (task->sign == SIGN_NAME && task->thread_name == curr_thread_name)
					|| (task->sign == SIGN_TID && task->thread_tid == curr_thread_tid)) {
					m_currSc_task = std::move(*it);
					m_coTasks.erase(it);
					break;
				} else {
					remind();
				}
			}

			locker.unlock();

			if(m_currSc_task) {
				if(m_currSc_task->timeout < now_time
					&& m_mode == DISCARD) {
					m_currSc_task.reset();
					continue;
				}
				m_currSc_task->co_func->swapIn();
				if(m_currSc_task
						 && m_currSc_task->co_func->getState() == Coroutine::HOLD) {
					if(m_currSc_task->sign == SIGN_ANY) {
						schedule(m_currSc_task->co_func);
					} else if(m_currSc_task->sign == SIGN_NAME) {
						schedule(m_currSc_task->thread_name, m_currSc_task->co_func);
					} else {		// TID
						schedule(m_currSc_task->thread_tid, m_currSc_task->co_func);
					}
				}				

				m_currSc_task.reset();
				--m_taskCount;
			}
		} else {
			locker.unlock();
			++m_waitingThreadCount;
			wait_coFunc->swapIn();
			--m_waitingThreadCount;
			
			wait_coFunc->reset();
		}
	}

	//WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
	//	<< m_name << " thread_name : " 
	//	<< curr_thread_name << "  tid : " << curr_thread_tid
	//	<< " over";
}

std::string now_scheduler::GetSchedulerName() {
	if(!Scheduler::m_currScheduler) {
		return "UNKNOWON_SCHEDULER";
	}
	return Scheduler::m_currScheduler->m_name;
}

uint64_t now_scheduler::GetSchedulerTimeout() {
	if(!Scheduler::m_currScheduler) {
		return Scheduler::ERR_TIMEOUT;
	}
	return Scheduler::m_currScheduler->m_timeout;
}

void now_scheduler::ReSchedule() {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr
			&& Scheduler::m_currSc_task != nullptr);
		
	Scheduler::m_currSc_task->sign = Scheduler::Task_Sign::SIGN_ANY; 
	now_coroutine::SwapOut();
}

void now_scheduler::ReSchedule(const std::string& thread_name) {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr
			&& Scheduler::m_currSc_task != nullptr);
	
	Scheduler::m_currSc_task->sign = Scheduler::Task_Sign::SIGN_NAME; 
	Scheduler::m_currSc_task->thread_name = thread_name; 
	now_coroutine::SwapOut();
}

void now_scheduler::ReSchedule(pid_t thread_tid) {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr
			&& Scheduler::m_currSc_task != nullptr);
	
	Scheduler::m_currSc_task->sign = Scheduler::Task_Sign::SIGN_TID;
	Scheduler::m_currSc_task->thread_tid = thread_tid;
	now_coroutine::SwapOut();
}

void now_scheduler::Schedule(Coroutine::ptr co_func) {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr);
	Scheduler::m_currScheduler->schedule(co_func);
}

void now_scheduler::Schedule(const std::string& thread_name, Coroutine::ptr co_func) {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr);
	Scheduler::m_currScheduler->schedule(thread_name, co_func);
}

void now_scheduler::Schedule(pid_t thread_tid, Coroutine::ptr co_func) {
	WEBSERVER_ASSERT(Scheduler::m_currScheduler != nullptr);
	Scheduler::m_currScheduler->schedule(thread_tid, co_func);
}

void now_scheduler::Schedule(std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	Schedule(co_func);
}

void now_scheduler::Schedule(const std::string& thread_name, std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	Schedule(thread_name, co_func);
}

void now_scheduler::Schedule(pid_t thread_tid, std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	Schedule(thread_tid, co_func);
}

}	// namespace webserver
