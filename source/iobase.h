#ifndef __WEBSERVER_IOBASE_H__
#define __WEBSERVER_IOBASE_H__

#include <memory>
#include <atomic>
#include <functional>
#include <map>

#include "coroutine.h"
#include "tools/scheduler.h"
#include "tools/timer.h"
#include "synchronism.h"

namespace webserver {

class IOBase : public Scheduler, public TimerManager {
public:
	using ptr = std::shared_ptr<IOBase>;
	using MutexType = Mutex;
	
	enum EType {
		NONE 	= 0x000,
		READ 	= 0x001,		
		WRITE 	= 0x004		
	};

	IOBase(const std::string& name, uint32_t thread_num,
           TimeoutMode mode = TRIGGER, uint64_t timeout = 3000);
	virtual ~IOBase();
	
	//scheduler
	//virtual void wait() {};
	//virtual void remind() {};
	
	virtual void start() = 0;
	virtual void stop() = 0;

	virtual bool addEvent(int fd, EType event, std::function<void()> func) = 0;
	virtual bool addEvent(int fd, EType event, Coroutine::ptr co_func) = 0;
	virtual bool delEvent(int fd, EType event) = 0;
	virtual void delAll(int fd) = 0;
	virtual void delAll() = 0;
	virtual bool triggerEvent(int fd, EType event) = 0;
	virtual void triggerAll(int fd) = 0;
	virtual void triggerAll() = 0;
	
	uint32_t eventCount() const { return m_eventCount.load(); }

private:
	bool condStatisfy() override final;
	void insertOnFirst() override final;

protected:
	struct FdEvent {
		using ptr = std::shared_ptr<FdEvent>;
		using MutexType = Mutex;
	
		FdEvent(int fd);
		~FdEvent();
		
		int fd;	
		uint32_t events;
		mutable MutexType mtx;
		Coroutine::ptr readEvent;
		Coroutine::ptr writeEvent;
	};

protected:
	FdEvent::ptr getFdEventNoLock(int fd);
	FdEvent::ptr getFdEvent(int fd);

protected:
	mutable MutexType m_mtx;
	std::atomic<uint32_t> m_eventCount;
	std::map<int, FdEvent::ptr> m_fdEvents;
};

}	// namespace webserver

#endif // !__WEBSERVER_IOBASE_H__
