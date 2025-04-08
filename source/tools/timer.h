#ifndef __WEBSERVER_TIMER_H__
#define __WEBSERVER_TIMER_H__

#include <memory>
#include <set>
#include <vector>
#include <functional>

#include "synchronism.h"

namespace webserver {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
	friend class TimerManager;
public:
	using ptr = std::shared_ptr<Timer>;
	
	~Timer() = default;
	
	void cancel();
	bool refresh();
	
	bool is_loop() const { return m_loop; }
	uint64_t getInterval() const { return m_interval; }
	std::function<void()> getFunc() const { return m_func; }

private:
	Timer(TimerManager* mgr, uint64_t interval_ms, 
			std::function<void()> func, bool loop = false);

	Timer();

private:
	struct Compare {
		bool operator()(const Timer::ptr rhs, const Timer::ptr lhs) const;
	};

private:
	bool m_loop;
	uint64_t m_next;
	uint64_t m_interval;
	std::function<void()> m_func;
	TimerManager* m_mgr;

public:
	static uint64_t MAX_TIME;
};


class TimerManager {
	friend class Timer;
public:
	using ptr = std::shared_ptr<TimerManager>;
	using MutexType = Mutex;

	TimerManager();
	virtual ~TimerManager() = default;

	Timer::ptr addTimer(uint64_t interval_ms, 
			std::function<void()> func, bool loop = false);
	Timer::ptr addConditionTimer(uint64_t interval_ms, 
			std::function<void()> func, std::weak_ptr<void> weak_cond, bool loop = false);
	uint64_t getNextTime();
	void clearTimers();
	
	void getTermTimers(std::vector<Timer::ptr>& vec);
	uint32_t timerCount() const;
	
	virtual void insertOnFirst() = 0;

private:
	void addTimer(Timer::ptr timer);

private:
	mutable MutexType m_mtx;
	std::multiset<Timer::ptr, Timer::Compare> m_timers;
};

//TODO check system clock change

} 	// namespace webserver

#endif // ! __WEBSERVER_TIMER_H__
