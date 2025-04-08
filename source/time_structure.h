#ifndef __WEBSERVER_TIME_H__
#define __WEBSERVER_TIME_H__

#include <memory>
#include <sys/time.h>
#include <string.h>

namespace webserver {

class Timespec {
public:
	using ptr = std::shared_ptr<Timespec>;
	
	Timespec();
	Timespec(time_t sec, long nsec);
	
	~Timespec() = default;
	
	time_t seconds() const { return m_timespec.tv_sec; }
	long nanoseconds() const { return m_timespec.tv_nsec; } 
	
	void setSeconds(time_t sec) { m_timespec.tv_sec = sec; }
	void setNanoseconds(long nsec) { m_timespec.tv_nsec = nsec; }
	
	const struct timespec* get() const { return &m_timespec; }
	struct timespec* get() { return &m_timespec; }

	uint32_t getLenth() const { return sizeof(m_timespec); }
		
private:
	struct timespec m_timespec;
};

class Timeval {
public:
	using ptr = std::shared_ptr<Timeval>;
	
	Timeval();
	Timeval(time_t sec, suseconds_t usec);
	
	~Timeval() = default;
	
	time_t seconds() const { return m_timeval.tv_sec; }
	suseconds_t microseconds() const { return m_timeval.tv_usec; }

	void setSeconds(time_t sec) { m_timeval.tv_sec = sec; }
	void setMicroseconds(suseconds_t usec) { m_timeval.tv_usec = usec; }
	
	const struct timeval* get() const { return &m_timeval; }
	struct timeval* get() { return &m_timeval; }
	
	uint32_t getLenth() const { return sizeof(m_timeval); }

private:
	struct timeval m_timeval;
};

}	//namespace webserver

#endif // !__WEBSERVER_TIME_H__
