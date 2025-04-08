#include "epollio.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace webserver {

constexpr const char* g_logger = "system";

//EpollIO
EpollIO::EpollIO(const std::string& name, uint32_t thread_num,
			TimeoutMode mode, uint64_t timeout)
	: IOBase(name, thread_num, mode, timeout)
	, m_isStop(true), m_epollfd(-1){
	m_tickles[0] = -1;
	m_tickles[1] = -1;
}

EpollIO::~EpollIO() = default;

void EpollIO::start() {
	if(!m_isStop) {
		return;
	}
	m_isStop = false;

	m_epollfd = epoll_create(1024);
	WEBSERVER_ASSERT2(m_epollfd != -1, "EpollIO::EpollIO -- epoll_create error");
	
	int32_t rt = pipe(m_tickles);
	WEBSERVER_ASSERT2(!rt, "EpollIO::EpollIO -- pipe error");
		
	int32_t flag = fcntl(m_tickles[0], F_GETFL, 0);
	fcntl(m_tickles[0], F_SETFL, flag | O_NONBLOCK);
	
	FdEvent::ptr fd_event = getFdEventNoLock(m_tickles[0]);
	
	struct epoll_event ep_event;
	memset(&ep_event, 0x00, sizeof(ep_event));
	ep_event.events = READ | EPOLLET;
	ep_event.data.ptr = fd_event.get();

	rt = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_tickles[0], &ep_event);
	WEBSERVER_ASSERT2(!rt, "EpollIO::EpollIO -- pipe error");
	
	Scheduler::start();
}

void EpollIO::stop() {
	if(m_isStop) {
		return;
	}
	m_isStop = true;

	triggerAll();
	TimerManager::clearTimers();
	Scheduler::stop();

	close(m_epollfd);
	close(m_tickles[0]);
	close(m_tickles[1]);
}

bool EpollIO::assisant_addEvent(int fd, IOBase::EType event, Coroutine::ptr co_func) {
	if(fd < 0 || !co_func) { return false; }
	
	FdEvent::ptr fd_event = getFdEvent(fd);	

	FdEvent::MutexType::Lock locker(fd_event->mtx);
	if(event & fd_event->events) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::assisants_addEvent invalid argument"
			<< "  event = " << event;
		return false;
	}
		
	struct epoll_event ep_event;
	memset(&ep_event, 0x00, sizeof(ep_event));
	ep_event.events = fd_event->events | event | EPOLLET;
	ep_event.data.ptr = fd_event.get();

	int op = fd_event->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
	int32_t rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::assisant_addEvent -- epoll_ctl error";
		return false;
	}
	
	fd_event->events |= event;
	if(event & READ) {
		fd_event->readEvent = co_func;
	}
	if(event & WRITE) {
		fd_event->writeEvent = co_func;
	}
	++m_eventCount;
	
	return true;
}

bool EpollIO::addEvent(int fd, IOBase::EType event, std::function<void()> func) {
	Coroutine::ptr co_func(new Coroutine(func));
	return assisant_addEvent(fd, event, co_func);
}

bool EpollIO::addEvent(int fd, IOBase::EType event, Coroutine::ptr co_func) {
	return assisant_addEvent(fd, event, co_func);
}

bool EpollIO::delEvent(int fd, IOBase::EType event) {
	if(fd < 0) { return false; }
	
	FdEvent::ptr fd_event = getFdEvent(fd);
	
	FdEvent::MutexType::Lock locker(fd_event->mtx);
	if(!(event & fd_event->events)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::delEvent invalid argument"
			<< "  event = " << event;
		return false;
	}
		
	uint32_t real_events = fd_event->events;
	if(event & READ) {
		real_events &= ~READ;
	}
	if(event & WRITE) {
		real_events &= ~WRITE;
	}
	
	struct epoll_event ep_event;
	memset(&ep_event, 0x00, sizeof(ep_event));
	ep_event.events = real_events;
	ep_event.data.ptr = fd_event.get();

	int op = real_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
	int32_t rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::delEvent epoll_ctl error"
			<< "  event = " << event;
		return false;
	}
	
	fd_event->events = real_events;
	if(event & READ) {
		fd_event->readEvent.reset();
	}
	if(event & WRITE) {
		fd_event->writeEvent.reset();
	}
	--m_eventCount;
	
	return true;
}

void EpollIO::delAll(int fd) {
	if(fd < 0) { return; }
	
	FdEvent::ptr fd_event = getFdEvent(fd);
	
	FdEvent::MutexType::Lock locker(fd_event->mtx);
	if(fd_event->events == NONE) {
		return;
	}

	int op = EPOLL_CTL_DEL;
	int32_t rt = epoll_ctl(m_epollfd, op, fd, nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::delAll epoll_ctl error"
			<< "   fd : " << fd << " events : " << fd_event->events
			<< " " << fd_event->fd;
		return;
	}
	
	if(fd_event->events & READ) {
		fd_event->readEvent.reset();
		--m_eventCount;
	}
	if(fd_event->events & WRITE) {
		fd_event->writeEvent.reset();
		--m_eventCount;
	}
	fd_event->events = NONE;
}

void EpollIO::delAll() {
	for(auto& it : m_fdEvents) {
		delAll(it.first);
	}
}


bool EpollIO::triggerEvent(int fd, IOBase::EType event) {
	if(fd < 0) { return false; }
	
	FdEvent::ptr fd_event = getFdEvent(fd);
	
	FdEvent::MutexType::Lock locker(fd_event->mtx);
	if(!(event & fd_event->events)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::triggerEvent invalid argument"
			<< "  event = " << event;
		return false;
	}
		
	uint32_t real_events = fd_event->events;
	if(event & READ) {
		real_events &= ~READ;
	}
	if(event & WRITE) {
		real_events &= ~WRITE;
	}
	
	struct epoll_event ep_event;
	memset(&ep_event, 0x00, sizeof(ep_event));
	ep_event.events = real_events;
	ep_event.data.ptr = fd_event.get();

	int op = real_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
	int32_t rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::triggerEvent epoll_ctl error";
		return false;
	}
	
	fd_event->events = real_events;
	if(event & READ) {
		 schedule(fd_event->readEvent);
		fd_event->readEvent.reset();
	}
	if(event & WRITE) {
		schedule(fd_event->writeEvent);
		fd_event->writeEvent.reset();
	}
	--m_eventCount;
	
	return true;
}

void EpollIO::triggerAll(int fd) {
	if(fd < 0) { return; }
	
	FdEvent::ptr fd_event = getFdEvent(fd);
	
	FdEvent::MutexType::Lock locker(fd_event->mtx);
	if(fd_event->events == NONE) {
		return;
	}

	int op = EPOLL_CTL_DEL;
	int32_t rt = epoll_ctl(m_epollfd, op, fd, nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		WEBSERVER_LOG_DEBUG(WEBSERVER_LOGMGR_GET(g_logger))
			<< "EpollIO::triggerAll epoll_ctl error"
			<< "   fd : " << fd << " events : " << fd_event->events
			<< " " << fd_event->fd;
		return;
	}
	
	if(fd_event->events & READ) {
		schedule(fd_event->readEvent);
		fd_event->readEvent.reset();
		--m_eventCount;
	}
	if(fd_event->events & WRITE) {
		schedule(fd_event->writeEvent);
		fd_event->writeEvent.reset();
		--m_eventCount;
	}
	fd_event->events = NONE;
}

void EpollIO::triggerAll() {
	for(auto& it : m_fdEvents) {
		triggerAll(it.first);
	}
}

void EpollIO::wait() {
	std::unique_ptr<struct epoll_event[]> ep_events(new struct epoll_event[1024]);
	
	int32_t ev_count = 0;
	while(true) {
		static int32_t EP_MAX_TIMEOUT = 3000;

		uint64_t next_time = getNextTime();
		uint64_t now_time = GetMs();
		if(next_time == Timer::MAX_TIME) { next_time = EP_MAX_TIMEOUT; }
		else if(next_time > EP_MAX_TIMEOUT + now_time) { next_time = EP_MAX_TIMEOUT; }
		else { 
			if(next_time > now_time) {
				next_time = next_time - now_time;
			} else {
				next_time = 0;
			}
		}
		
		//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_GET(g_logger))
		//	<< "next_time : " << next_time << "   "
		//	<< "TimerCount : " << timerCount() << "   "
		//	<< "eventCount : " << eventCount() << "   "
		//	<< "taskCount : " << taskCount();
	
		ev_count = epoll_wait(m_epollfd, ep_events.get(), 1024, static_cast<int32_t>(next_time));
		if(ev_count == -1) {
			if(errno == EINTR) {
				continue;
			}
			WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET(g_logger))
				<< "EpollIO::wait -- epoll_wait error  what : "
				<< strerror(errno);
			::abort();
		} else {
			break;
		}
	}
	
	std::vector<Timer::ptr> term_timers;
	getTermTimers(term_timers);
	for(size_t i = 0; i < term_timers.size(); ++i) {
		schedule(term_timers[i]->getFunc());
	}

	for(int32_t i = 0; i < ev_count; ++i) {
		FdEvent* fd_event = static_cast<FdEvent*>(ep_events[i].data.ptr);
		if(fd_event->fd == m_tickles[0]) {
			char buff[1024] = {0};
			while(read(m_tickles[0], buff, sizeof(buff)) != -1);
		
			continue;
		}
		
		uint32_t events = ep_events[i].events;
		if(events & READ) {
			triggerEvent(fd_event->fd, READ);
		}
		if(events & WRITE) {
			triggerEvent(fd_event->fd, WRITE);
		}
	}
}
 
void EpollIO::remind() {
	if(waitingThreadCount()) {
		int32_t rt = write(m_tickles[1], "T", sizeof("T"));
		WEBSERVER_ASSERT2(rt == sizeof("T"), "EpollIO::remind -- write error");
	}
}

}	// namespace webserver
