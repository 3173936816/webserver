#ifndef __WEBSERVER_ASYNC_H__
#define __WEBSERVER_ASYNC_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "noncopyable.h"

namespace webserver {

class async : Noncopyable {
public:
	
	// set async
	static void setAsync(bool v) { m_isAsync = v; }
	static bool isAsync() { return m_isAsync; }
	
	//sleep related
	static unsigned int Sleep(unsigned int seconds);
	static int Usleep(useconds_t usec);

	//socket related
	static int Socket(int domain, int type, int protocol);
	static int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	static int Listen(int sockfd, int backlog);	
	static int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	static int Connect_with_timeout(int sockfd, const struct sockaddr *addr
							, socklen_t addrlen, uint64_t timeout);
	static int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	static int Close(int fd);

	//read related
	static ssize_t Read(int fd, void *buf, size_t count);	
	static ssize_t Readv(int fd, const struct iovec *iov, int iovcnt);
	static ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
	static ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags,
    	                    struct sockaddr *src_addr, socklen_t *addrlen);
	static ssize_t Recvmsg(int sockfd, struct msghdr *msg, int flags);
	
	//write related
	static ssize_t Write(int fd, const void *buf, size_t count);
	static ssize_t Writev(int fd, const struct iovec *iov, int iovcnt);
	static ssize_t Send(int sockfd, const void *buf, size_t len, int flags);
	static ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
    					const struct sockaddr *dest_addr, socklen_t addrlen);
	static ssize_t Sendmsg(int sockfd, const struct msghdr *msg, int flags);
	
	//other related
    static int Fcntl(int fd, int cmd, ...);
	static int Getsockopt(int sockfd, int level, int optname,
    					void *optval, socklen_t *optlen);
    static int Setsockopt(int sockfd, int level, int optname,
    					const void *optval, socklen_t optlen);
    static int Ioctl(int d, int request, ...);

private:
	enum OPT {
		OPT_READ = 0,
		OPT_WRITE = 1
	};	

	template <typename Func, typename... Args>
	static ssize_t assistant_io(int sockfd, Func func
		, OPT opt, const char* funcName, Args&&... args);

private:
	async() = delete;
	~async() = delete;

private:
	static thread_local bool m_isAsync;
};

}	// namespace webserver

#endif // __WEBSERVER_ASYNC_H__
