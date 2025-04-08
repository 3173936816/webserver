#include "timer.h"
#include "macro.h"
#include "util.h"

#include <algorithm>

namespace webserver {

uint64_t Timer::MAX_TIME = ~0x0ull;

//Timer
bool Timer::Compare::operator()(const Timer::ptr lhs, const Timer::ptr rhs) const {
	WEBSERVER_ASSERT(lhs != nullptr && rhs != nullptr);

	return lhs->m_next < rhs->m_next;
}

Timer::Timer(TimerManager* mgr, uint64_t interval_ms, 
		std::function<void()> func, bool loop)
	: m_loop(loop), m_next(GetMs() + interval_ms) 
	, m_interval(interval_ms), m_func(func) 
	, m_mgr(mgr) {
}

Timer::Timer()
	: m_loop(false), m_next(GetMs())
	, m_interval(0), m_func{}
	, m_mgr(nullptr) {
}

void Timer::cancel() {
	Timer::ptr self = shared_from_this();
	TimerManager::MutexType::Lock locker(m_mgr->m_mtx); 
	auto it = std::find_if(m_mgr->m_timers.begin(), m_mgr->m_timers.end(), 
			[this](Timer::ptr timer)->bool {
		return timer.get() == this;
	});
	if(it != m_mgr->m_timers.end()) {
		m_mgr->m_timers.erase(it);
	}
}

bool Timer::refresh() {
	Timer::ptr self = shared_from_this();

	TimerManager::MutexType::Lock locker(m_mgr->m_mtx); 
	auto it = std::find_if(m_mgr->m_timers.begin(), m_mgr->m_timers.end(), 
			[this](Timer::ptr timer)->bool {
		return timer.get() == this;
	});
	if(it == m_mgr->m_timers.end()) {
		return false;
	}
	m_mgr->m_timers.erase(it);
	m_next = GetMs() + m_interval;

	m_mgr->m_timers.insert(self);
	if(self->m_next == (*(m_mgr->m_timers.begin()))->m_next) {
		m_mgr->insertOnFirst();
	}
	
	return true;
}
	
//TimerManager
TimerManager::TimerManager()
	: m_mtx{}, m_timers{} {
}

Timer::ptr TimerManager::addTimer(uint64_t interval_ms, 
			std::function<void()> func, bool loop) {
	Timer::ptr new_timer(new Timer(this, interval_ms, func, loop));

	MutexType::Lock locker(m_mtx);
	m_timers.insert(new_timer);
	if(new_timer->m_next == (*m_timers.begin())->m_next) {
		insertOnFirst();
	}
	
	return new_timer;
}

static void assistant_func(std::function<void()> func,
		 std::weak_ptr<void> weak_cond) {
	auto it = weak_cond.lock();
	if(it) {
		func();
	}
}

Timer::ptr TimerManager::addConditionTimer(uint64_t interval_ms, 
		std::function<void()> func, std::weak_ptr<void> weak_cond, bool loop) {
	Timer::ptr new_timer(new Timer(this, interval_ms, 
			std::bind(assistant_func, func, weak_cond), loop));
	MutexType::Lock locker(m_mtx);
	m_timers.insert(new_timer);
	if(new_timer->m_next == (*m_timers.begin())->m_next) {
		insertOnFirst();
	}
	
	return new_timer;
}

void TimerManager::addTimer(Timer::ptr timer) {
	MutexType::Lock locker(m_mtx);
	m_timers.insert(timer);
	if(timer->m_next == (*m_timers.begin())->m_next) {
		insertOnFirst();
	}
}

uint64_t TimerManager::getNextTime() {
	MutexType::Lock locker(m_mtx);
	if(m_timers.empty()) {
		return Timer::MAX_TIME;
	}
	auto it = m_timers.begin();
	return (*it)->m_next;
}

void TimerManager::clearTimers() {
	MutexType::Lock locker(m_mtx);
	m_timers.clear();
}

void TimerManager::getTermTimers(std::vector<Timer::ptr>& vec) {
	MutexType::Lock locker(m_mtx);
	Timer::ptr now_timer(new Timer());
	
	auto upper_it = m_timers.upper_bound(now_timer);
	for(auto it = m_timers.begin(); it != upper_it; ++it) {
		vec.push_back(*it);
	}
	m_timers.erase(m_timers.begin(), upper_it);

	for(auto& it : vec) {
		if(it->is_loop()) {
			it->m_next += it->m_interval;
			m_timers.insert(it);
			if(it->m_next == (*m_timers.begin())->m_next) {
				insertOnFirst();
			}
		}
	}
}

uint32_t TimerManager::timerCount() const {
	MutexType::Lock locker(m_mtx);
	return m_timers.size();
}

}	// namespace webserver
