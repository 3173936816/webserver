#include "async.h"
#include "thread.h"
#include "coroutine.h"
#include "iobase.h"
#include "tools/sockfdinfo.h"
#include "log.h"

#include <stdarg.h>
#include <linux/version.h>

namespace webserver {

static constexpr const char* g_logger = "system";

thread_local bool async::m_isAsync = false;

//sleep related
unsigned int async::Sleep(unsigned int seconds) {	
	if(!m_isAsync) {
		return ::sleep(seconds);
	}
	
	IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
	if(!ioBase) {
		errno = EINVAL;
		return -1;
	}

	Scheduler::Sc_task::ptr task{nullptr};	
	task.swap(Scheduler::m_currSc_task);
	pid_t thread_tid = now_thread::GetTid();
	
	ioBase->addTimer(seconds * 1000, [ioBase, task, thread_tid]() {
		ioBase->schedule(thread_tid, task->co_func);
	});
	
	now_coroutine::SwapOut();
	return 0;
}

int async::Usleep(useconds_t usec) {
	if(!m_isAsync) {
		return ::usleep(usec);
	}
	
	IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
	if(!ioBase) {
		errno = EINVAL;
		return -1;
	}

	Scheduler::Sc_task::ptr task{nullptr};	
	task.swap(Scheduler::m_currSc_task);
	pid_t thread_tid = now_thread::GetTid();
	
	ioBase->addTimer(usec / 1000, [ioBase, task, thread_tid]() {
		ioBase->schedule(thread_tid, task->co_func);
	});
	
	now_coroutine::SwapOut();
	return 0;
}

//socket related
int async::Socket(int domain, int type, int protocol) {
	if(!m_isAsync) {
		return ::socket(domain, type, protocol);
	}
	int sockfd = ::socket(domain, type, protocol);
	SockFdInfo::ptr sock_info = SockFdInfoMgr::GetInstance()->addSockFdInfo(sockfd);
	if(!sock_info) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< sockfd << " invalid";
		return -1;
	}
	
	return sockfd;
}

int async::Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return ::bind(sockfd, addr, addrlen); 
}

int async::Listen(int sockfd, int backlog) {
	return ::listen(sockfd, backlog); 
}

int async::Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	if(!m_isAsync) {
		return ::accept(sockfd, addr, addrlen);
	}
	
	SockFdInfo::ptr sock_info =
		 SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(sock_info->getUserNonBlock()) {
		return ::accept(sockfd, addr, addrlen);
	}

retry:
	int new_sock = ::accept(sockfd, addr, addrlen);	
	if(new_sock == -1 && errno == EINTR) {
		goto retry;
	}
	
	if(new_sock == -1 && errno == EAGAIN) {
		IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
		if(!ioBase) {
			errno = EINVAL;
			return -1;
		}

		Scheduler::Sc_task::ptr task{nullptr};	
		task.swap(Scheduler::m_currSc_task);
		pid_t thread_tid = now_thread::GetTid();
		
		bool rt = ioBase->addEvent(sockfd, IOBase::READ, [ioBase, task, thread_tid](){
			ioBase->schedule(thread_tid, task->co_func);
		});
		if(!rt) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "Func : " << "accept" << " addEvent error"
				<< "  Socket : " << sockfd;
			errno = EINVAL;
			return -1;
		}
	
		now_coroutine::SwapOut();
		
		new_sock = ::accept(sockfd, addr, addrlen);
		if(new_sock < 0) {
			errno = EIO;
			return -1;
		}
	}
	
	SockFdInfo::ptr new_sock_info = SockFdInfoMgr::GetInstance()->addSockFdInfo(new_sock);
	if(!new_sock_info) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
			<< "accept : " << new_sock << " invalid";
		errno = EBADF;
		return -1;
	}	

	return new_sock;
}

static int select_connect_timeout(int sockfd
		, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout) {
	int flag = ::fcntl(sockfd, F_GETFL, 0);
	if(flag & O_NONBLOCK) {
		return ::connect(sockfd, addr, addrlen);
	}

	::fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);

	int rt = ::connect(sockfd, addr, addrlen);
	if(rt == -1 && errno == EINPROGRESS) {
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);
		
		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		
		int ret = select(sockfd + 1, nullptr, &fdset, nullptr, &tv);
		::fcntl(sockfd, F_SETFL, flag);

		if(ret == 0) {
			errno = ETIMEDOUT;
			return -1;
		} else if(ret < 0) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "select_connect_timeout --- select error : "
				<< strerror(errno);
			return -1;
		}
	} else {
		::fcntl(sockfd, F_SETFL, flag);
		return rt;
	}
	
	::fcntl(sockfd, F_SETFL, flag);

	int error = 0;
	socklen_t len = sizeof(int);
	if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "select_connect_timeout --- getsockopt error : "
				<< strerror(errno);
		return -1;
	}
	if(error) {
		errno = error;
		return -1;
	}
	errno = 0;
	return 0;
}

int async::Connect_with_timeout(int sockfd
		, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout) {
	if(!m_isAsync) {
		return select_connect_timeout(sockfd, addr, addrlen, timeout);
	}
	
	SockFdInfo::ptr sock_info =
		 SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(sock_info->getUserNonBlock()) {
		return select_connect_timeout(sockfd, addr, addrlen, timeout);
	}

retry:	
	int rt = ::connect(sockfd, addr, addrlen);
	if(rt == -1 && errno == EINTR) {
		goto retry;
	}

	if(rt == -1 && errno == EINPROGRESS) {
		IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
		if(!ioBase) {
			errno = EINVAL;
			return -1;
		}

		Scheduler::Sc_task::ptr task{nullptr};	
		task.swap(Scheduler::m_currSc_task);
		pid_t thread_tid = now_thread::GetTid();
		
		bool ret = ioBase->addEvent(sockfd, IOBase::WRITE, [ioBase, task, thread_tid](){
			ioBase->schedule(thread_tid, task->co_func);
		});
		if(!ret) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "Func : " << "connect_with_timeout" << " addEvent error"
				<< "  Socket : " << sockfd;
			errno = EINVAL;
			return -1;
		}
		
		bool is_timeout = false;
		Timer::ptr timer = ioBase->addTimer(timeout, [ioBase, sockfd, &is_timeout]() mutable {
			is_timeout = ioBase->triggerEvent(sockfd, IOBase::WRITE);
		});
		
		now_coroutine::SwapOut();
		
		if(is_timeout) {
			errno = ETIMEDOUT;
			return -1;
		}
		
		timer->cancel();
	}
	
	int error = 0;
	socklen_t len = sizeof(int);
	if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "connect_with_timeout --- getsockopt error : "
				<< strerror(errno);
		return -1;
	}
	if(error) {
		errno = error;
		return -1;
	}
	errno = 0;
	return 0;
}

int async::Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	if(!m_isAsync) {
		return ::connect(sockfd, addr, addrlen);
	}

	SockFdInfo::ptr sock_info =
		 SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(sock_info->getUserNonBlock()) {
		return ::connect(sockfd, addr, addrlen);
	}

retry:	
	int rt = ::connect(sockfd, addr, addrlen);
	if(rt == -1 && errno == EINTR) {
		goto retry;
	}

	if(rt == -1 && errno == EINPROGRESS) {
		IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
		if(!ioBase) {
			errno = EINVAL;
			return -1;
		}

		Scheduler::Sc_task::ptr task{nullptr};	
		task.swap(Scheduler::m_currSc_task);
		pid_t thread_tid = now_thread::GetTid();
		
		bool ret = ioBase->addEvent(sockfd, IOBase::WRITE, [ioBase, task, thread_tid](){
			ioBase->schedule(thread_tid, task->co_func);
		});
		if(!ret) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "Func : " << "connect" << " addEvent error"
				<< "  Socket : " << sockfd;
			errno = EINVAL;
			return -1;
		}
		
		now_coroutine::SwapOut();
	}
	
	int error = 0;
	socklen_t len = sizeof(int);
	if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "connect --- getsockopt error : "
				<< strerror(errno);
		return -1;
	}
	if(error) {
		errno = error;
		return -1;
	}
	errno = 0;
	return 0;
}

int async::Close(int fd) {
	if(!m_isAsync) {
		return ::close(fd);
	}
	
	SockFdInfo::ptr sock_info =
		 SockFdInfoMgr::GetInstance()->getSockFdInfo(fd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	sock_info->setClose(true);

	return ::close(fd);
}

template <typename Func, typename... Args>
ssize_t async::assistant_io(int sockfd, Func func
		, OPT opt, const char* funcName, Args&&... args) {
	if(!m_isAsync) {
		return func(sockfd, std::forward<Args>(args)...);
	}
	
	SockFdInfo::ptr sock_info =
		 SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(sock_info->getUserNonBlock()) {
		return func(sockfd, std::forward<Args>(args)...);
	}
	
retry:
	int rt = func(sockfd, std::forward<Args>(args)...);
	if(rt == -1 && errno == EINTR) {
		goto retry;
	}

	if(rt == -1 && errno == EAGAIN) {
		IOBase* ioBase = dynamic_cast<IOBase*>(Scheduler::m_currScheduler);
		if(!ioBase) {
			errno = EINVAL;
			return -1;
		}

		Scheduler::Sc_task::ptr task{nullptr};	
		task.swap(Scheduler::m_currSc_task);
		pid_t thread_tid = now_thread::GetTid();
		uint64_t time_msg = ~0x0ull;
		IOBase::EType etype;
		if(opt == OPT_READ) {
			time_msg = sock_info->getRecvTimeout();
			etype = IOBase::READ;
		} else if(opt == OPT_WRITE) {
			time_msg = sock_info->getSendTimeout();
			etype = IOBase::WRITE;
		} else {
			errno = EINVAL;
			return -1;
		}
	
		bool is_timeout = false;	
	
		bool res = ioBase->addEvent(sockfd, etype, [ioBase, task, thread_tid](){
			ioBase->schedule(thread_tid, task->co_func);
		});
		if(!res) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(g_logger))
				<< "Func : " << funcName << " addEvent error"
				<< "  Socket : " << sockfd;
			errno = EINVAL;
			return -1;
		}

		Timer::ptr timer = nullptr;
		if(time_msg != ~0x0ull) {
			timer = ioBase->addTimer(time_msg, 
					[ioBase, sockfd, &is_timeout, etype]() mutable {
				is_timeout = ioBase->triggerEvent(sockfd, etype);
			});
		}
		
		now_coroutine::SwapOut();
		
		if(is_timeout) {
			errno = ETIMEDOUT;
			return -1;
		}

		if(timer) {
			timer->cancel();
		}
		
		rt = func(sockfd, std::forward<Args>(args)...);
		if(rt < 0) {
			errno = EIO;
			return -1;
		}
	}

	return rt;
}


//read related
ssize_t async::Read(int fd, void *buf, size_t count) {
	return assistant_io(fd, ::read, OPT::OPT_READ, "read", buf, count);
}

ssize_t async::Readv(int fd, const struct iovec *iov, int iovcnt) {
	return assistant_io(fd, ::readv, OPT::OPT_READ, "readv", iov, iovcnt);
}

ssize_t async::Recv(int sockfd, void *buf, size_t len, int flags) {
	return assistant_io(sockfd, ::recv, OPT::OPT_READ, "recv", buf, len, flags);
}

ssize_t async::Recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
	return assistant_io(sockfd, ::recvfrom, OPT::OPT_READ, "recvfrom", buf, len, flags, src_addr, addrlen);
}

ssize_t async::Recvmsg(int sockfd, struct msghdr *msg, int flags) {
	return assistant_io(sockfd, ::recvmsg, OPT::OPT_READ, "recvmsg", msg, flags);
}

//write related
ssize_t async::Write(int fd, const void *buf, size_t count) {
	return assistant_io(fd, ::write, OPT::OPT_WRITE, "write", buf, count);
}

ssize_t async::Writev(int fd, const struct iovec *iov, int iovcnt) {
	return assistant_io(fd, ::writev, OPT::OPT_WRITE, "writev", iov, iovcnt);
}

ssize_t async::Send(int sockfd, const void *buf, size_t len, int flags) {
	return assistant_io(sockfd, ::send, OPT::OPT_WRITE, "send", buf, len, flags);
}

ssize_t async::Sendto(int sockfd, const void *buf, size_t len, int flags,
 			const struct sockaddr *dest_addr, socklen_t addrlen) {
	return assistant_io(sockfd, ::sendto, OPT::OPT_WRITE, "sendto", buf, len, flags, dest_addr, addrlen);
}

ssize_t async::Sendmsg(int sockfd, const struct msghdr *msg, int flags) {
	return assistant_io(sockfd, ::sendmsg, OPT::OPT_WRITE, "sendmsg", msg, flags);
}

//other related
int async::Fcntl(int fd, int cmd, ...) {
	va_list va_st;
	va_start(va_st, cmd);
	
	switch(cmd) {
		case F_GETFL: {		//	void
			va_end(va_st);

			if(!m_isAsync) {
				return ::fcntl(fd, cmd, 0);
			}

			int flag = ::fcntl(fd, cmd, 0);
			SockFdInfo::ptr sock_info =
		 		SockFdInfoMgr::GetInstance()->getSockFdInfo(fd);
			if(!sock_info) {
				errno = EBADF;
				return -1;
			}
			
			return sock_info->getUserNonBlock() ? flag : (flag & ~O_NONBLOCK);
		}
		case F_SETFL: {		//	int
			int args = va_arg(va_st, int);	
			va_end(va_st);
	
			if(!m_isAsync) {
				return ::fcntl(fd, cmd, args);
			}
			
			SockFdInfo::ptr sock_info =
		 		SockFdInfoMgr::GetInstance()->getSockFdInfo(fd);
			if(!sock_info) {
				errno = EBADF;
				return -1;
			}
			
			if(args & O_NONBLOCK) {
				sock_info->setUserNonBlock(true);
			}
			return ::fcntl(fd, cmd, args | O_NONBLOCK);
		}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
		case F_DUPFD_CLOEXEC:	// int 
#endif
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		case F_SETPIPE_SZ:	//	int
#endif 	
		case F_DUPFD:		// 	int
		case F_SETFD:		//	int
		case F_SETOWN:		//	int
		case F_SETSIG:		//	int
		case F_NOTIFY:  	//	int
		case F_SETLEASE: {	// 	int
			int args = va_arg(va_st, int);
			va_end(va_st);

			return ::fcntl(fd, cmd, args);
		}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		case F_GETPIPE_SZ:  //	void
#endif
		case F_GETFD:		//	void
		case F_GETOWN:		//	void
		case F_GETSIG:		//	void
		case F_GETLEASE: {	//	void
			va_end(va_st);
			return ::fcntl(fd, cmd, 0);
		}
	

		case F_SETLK:		//	struct flock *
		case F_SETLKW:		//	struct flock *
		case F_GETLK: {		//  struct flock *
			struct flock* args = va_arg(va_st, struct flock*);
			va_end(va_st);
			
			return ::fcntl(fd, cmd, args);
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)	
		case F_GETOWN_EX:	//	struct f_owner_ex *
		case F_SETOWN_EX: {	//	struct f_owner_ex *
			struct f_owner_ex* args = va_arg(va_st, struct f_owner_ex*);
			va_end(va_st);
			
			return ::fcntl(fd, cmd, args);
		}
#endif

	}
	
	va_end(va_st);
	errno = EINVAL;

	return -1;			
}

int async::Getsockopt(int sockfd, int level, int optname,
 			void *optval, socklen_t *optlen) {
	if(!m_isAsync) {
		return ::getsockopt(sockfd, level, optname, optval, optlen);
	}
	
	SockFdInfo::ptr sock_info =
			SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(level == SOL_SOCKET) {
		if(optname == SO_RCVTIMEO
				|| optname == SO_SNDTIMEO) {
			struct timeval* t = static_cast<struct timeval*>(optval);
			uint64_t time_msg = (optname == SO_RCVTIMEO) 
					? sock_info->getRecvTimeout()
					: sock_info->getSendTimeout();
			
			if(time_msg == ~0x0ull) {
				memset(t, 0x00, *optlen);
			} else {
				t->tv_sec = time_msg / 1000;
				t->tv_usec = (time_msg % 1000) * 1000;
			}
			return 0;
		}
	}

	return ::getsockopt(sockfd, level, optname, optval, optlen);
}

int async::Setsockopt(int sockfd, int level, int optname,
 			const void *optval, socklen_t optlen) {
	if(!m_isAsync) {
		return ::setsockopt(sockfd, level, optname, optval, optlen);
	}
	
	SockFdInfo::ptr sock_info =
			SockFdInfoMgr::GetInstance()->getSockFdInfo(sockfd);
	if(!sock_info) {
		errno = EBADF;
		return -1;
	}
	
	if(level == SOL_SOCKET) {
		if(optname == SO_RCVTIMEO
				|| optname == SO_SNDTIMEO) {
			const struct timeval* t = static_cast<const struct timeval*>(optval);
			uint64_t time_msg = t->tv_sec * 1000 + t->tv_usec / 1000;

			if(optname == SO_RCVTIMEO) {
				sock_info->setRecvTimeout(time_msg);
			} else {
				sock_info->setSendTimeout(time_msg);
			}
			return 0;
		}
	}

	return ::setsockopt(sockfd, level, optname, optval, optlen);
}

int async::Ioctl(int d, int request, ...) {
	va_list va_st;
	va_start(va_st, request);
	void* arg = va_arg(va_st, void*);
	va_end(va_st);

	if(!m_isAsync) {
		return ::ioctl(d, request, arg);
	}
	
	if(request == FIONBIO) {
		SockFdInfo::ptr sock_info =
			SockFdInfoMgr::GetInstance()->getSockFdInfo(d);
		if(!sock_info) {
			errno = EBADF;
			return -1;
		}
	
		sock_info->setUserNonBlock(!!(*(int*)arg));
		return 0;
	}
	return ::ioctl(d, request, arg);
}

} //  namespace wevserver
