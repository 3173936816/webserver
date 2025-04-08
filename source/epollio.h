#ifndef __WEBSERVER_EPOLLIO_H__
#define __WEBSERVER_EPOLLIO_H__

#include <memory>
#include <string>
#include <atomic>
#include <sys/epoll.h>

#include "iobase.h"

namespace webserver {

class EpollIO : public IOBase {
public:
	using ptr = std::shared_ptr<EpollIO>;

	EpollIO(const std::string& name, uint32_t thread_num,
			TimeoutMode mode = TRIGGER, uint64_t timeout = 3000);
	~EpollIO();
	
	void start() override;
	void stop() override;

	bool addEvent(int fd, IOBase::EType event, std::function<void()> func) override;
	bool addEvent(int fd, IOBase::EType event, Coroutine::ptr co_func) override;
	bool delEvent(int fd, IOBase::EType event) override;
	void delAll(int fd) override;
	void delAll() override;
	bool triggerEvent(int fd, EType event) override;
	void triggerAll(int fd) override;
	void triggerAll() override;

	void wait() override;
	void remind() override;

private:
	bool assisant_addEvent(int fd, IOBase::EType event, Coroutine::ptr co_read_func);
	
private:
	bool m_isStop;
	int m_epollfd;
	int m_tickles[2];
};

}	// namespace webserver

#endif // !__WEBSERVER_EPOLLIO_H__
