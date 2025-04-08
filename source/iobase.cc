#include "iobase.h"

namespace webserver {

//	IOBase::FdEvent
IOBase::FdEvent::FdEvent(int fd)
	: fd(fd), events(NONE), mtx{Mutex::CHECK}
	, readEvent{nullptr}, writeEvent{nullptr} {
}

IOBase::FdEvent::~FdEvent() = default;

//	IOBase
IOBase::FdEvent::ptr IOBase::getFdEventNoLock(int fd) {
	auto it = m_fdEvents.find(fd);
	if(it == m_fdEvents.end()) {
		m_fdEvents[fd] = std::make_shared<FdEvent>(fd);
	}
	return m_fdEvents[fd];
}

IOBase::~IOBase() = default;

IOBase::FdEvent::ptr IOBase::getFdEvent(int fd) {
	MutexType::Lock locker(m_mtx);
	return getFdEventNoLock(fd);
}

IOBase::IOBase(const std::string& name, 
		uint32_t thread_num, TimeoutMode mode, uint64_t timeout)
	: Scheduler(name, thread_num, mode, timeout)
	, TimerManager(), m_mtx{Mutex::CHECK}
	, m_eventCount(0), m_fdEvents{} {
}
	
bool IOBase::condStatisfy() {
	return Scheduler::condStatisfy()
		|| TimerManager::timerCount()
		|| m_eventCount.load();
}

void IOBase::insertOnFirst() {
	remind();
}

}	// namespace webserver
