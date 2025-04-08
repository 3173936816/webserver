#include "time_structure.h"

namespace webserver {

Timespec::Timespec()
	: m_timespec{} {
	memset(&m_timespec, 0x00, sizeof(m_timespec));
}

Timespec::Timespec(time_t sec, long nsec)
	: m_timespec{} {
	m_timespec.tv_sec = sec;
	m_timespec.tv_nsec = nsec;
}

Timeval::Timeval()
	: m_timeval{} {
	memset(&m_timeval, 0x00, sizeof(m_timeval));
}

Timeval::Timeval(time_t sec, suseconds_t usec)
	: m_timeval{} {
	m_timeval.tv_sec = sec;
	m_timeval.tv_usec = usec;
}

} // namespace webserver
