#ifndef __WEBSERVER_SYNCHRONISM_H__
#define __WEBSERVER_SYNCHRONISM_H__

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

#include "noncopyable.h"
#include "time_structure.h"
#include "exception.h"
#include <iostream>

namespace webserver {

/*信号量*/
class Semaphore : Noncopyable {
public:
	explicit Semaphore(uint32_t count = 0);
	~Semaphore();

	void notify();
	void wait();
	bool trywait();
	bool timedwait(const Timespec& ts);
	int32_t getValue();

private:
	sem_t m_sem;
};

struct TryLock_t {	
	TryLock_t() = default;
	~TryLock_t() = default;
}; 

struct Locked_t {
	Locked_t() = default;
	~Locked_t() = default;
};		

struct KeepLock_t {
	KeepLock_t() = default;
	~KeepLock_t() = default;
};	

constexpr TryLock_t TryLock{};
constexpr Locked_t Locked{};
constexpr KeepLock_t KeepLock{};

/*Mutex RAII*/
template <class T>
class MutexScopedLockImpl : Noncopyable {
public:

	MutexScopedLockImpl(T& mtx)
		: m_locked(false), m_mtx(mtx) {
		m_mtx.lock();
		m_locked = true;
	}
	
	MutexScopedLockImpl(T& mtx, TryLock_t)
		: m_locked(false), m_mtx(mtx) {
		m_locked = m_mtx.trylock();
	}

	MutexScopedLockImpl(T& mtx, Locked_t)
		: m_locked(true), m_mtx(mtx) {
	}
	
	MutexScopedLockImpl(T& mtx, KeepLock_t)
		: m_locked(false), m_mtx(mtx) {
	}

	~MutexScopedLockImpl() {
		if(m_locked) {
			m_mtx.unlock();
			m_locked = false;
		}
	}

	void lock() {
		if(m_locked) {
			throw Mutex_error("MutexScopedLockImpl::lock error");
		}
		m_mtx.lock();
		m_locked = true;
	}
	
	void unlock() {
		if(!m_locked) {
			throw Mutex_error("MutexScopedLockImpl::unlock error");
		}
		m_mtx.unlock();
		m_locked = false;
	}
	
	bool trylock() {
		if(m_locked) {
			return false;
		}
		m_locked = m_mtx.trylock();
		return m_locked;
	}
	
	bool timedlock(const Timespec& ts) {
		if(m_locked) {
			return false;
		}
		m_locked = m_mtx.timedlock(ts);
		return m_locked;
	}

	bool getOwns() const { return m_locked; }

private:
	bool m_locked;
	T& m_mtx;
};

class CondVariable;

/*互斥锁*/
class Mutex : Noncopyable {
	friend class CondVariable;
public:
	using Lock = MutexScopedLockImpl<Mutex>;

	enum Attr : int32_t {
		NORMAL 		= PTHREAD_MUTEX_NORMAL,			//常规属性
		CHECK  		= PTHREAD_MUTEX_ERRORCHECK,		//错误检测
		RECURSIVE 	= PTHREAD_MUTEX_RECURSIVE,		//递归属性
		DEFAULT 	= PTHREAD_MUTEX_DEFAULT			//默认属性
	};
	
	explicit Mutex(Attr attr = NORMAL);
	~Mutex();
	
	void lock();
	bool trylock();
	bool timedlock(const Timespec& ts);
	void unlock();
	Attr getAttr() const { return m_attr; }
	
private:
	Attr m_attr;
	pthread_mutex_t m_mtx;
};

/*读锁RAII*/
template <class T>
class RDMutexScopedLockImpl : Noncopyable {
public:

	RDMutexScopedLockImpl(T& rdmtx)
		: m_locked(false), m_rdmtx(rdmtx) {
		m_rdmtx.rdlock();
		m_locked = true;
	}

	RDMutexScopedLockImpl(T& rdmtx, TryLock_t)
		: m_locked(false), m_rdmtx(rdmtx) {
		m_locked = m_rdmtx.tryRdlock();
	}

	RDMutexScopedLockImpl(T& rdmtx, Locked_t)
		: m_locked(true), m_rdmtx(rdmtx) {
	}
	
	RDMutexScopedLockImpl(T& rdmtx, KeepLock_t)
		: m_locked(false), m_rdmtx(rdmtx) {
	}
	
	~RDMutexScopedLockImpl() {
		if(m_locked) {
			m_rdmtx.unlock();
			m_locked = false;
		}
	}
	
	void rdlock() {
		if(m_locked) {
			throw Mutex_error("RDMutexScopedLockImpl::rdlock error");
		}
		m_rdmtx.rdlock();
		m_locked = true;
	}

	bool tryRdlock() {
		if(m_locked) {
			return false;
		}
		m_locked = m_rdmtx.tryRdlock();
		return m_locked;
	}
	
	bool timedRdlock(const Timespec& ts) {
		if(m_locked) {
			return false;
		}
		m_locked = m_rdmtx.timedRdlock(ts);
		return m_locked;
	}

	void unlock() {
		if(!m_locked) {
			throw Mutex_error("RDMutexScopedLockImpl::unlock error");
		}
		m_rdmtx.unlock();
		m_locked = false;
	}
	
	bool getOwns() const { return m_locked; }

private:
	bool m_locked;
	T& m_rdmtx;
};

/*写锁RAII*/
template <class T>
class WRMutexScopedLockImpl : Noncopyable {
public:

	WRMutexScopedLockImpl(T& wrmtx)
		: m_locked(false), m_wrmtx(wrmtx) {
		m_wrmtx.wrlock();
		m_locked = true;
	}

	WRMutexScopedLockImpl(T& wrmtx, TryLock_t)
		: m_locked(false), m_wrmtx(wrmtx) {
		m_locked = m_wrmtx.tryWrlock();
	}

	WRMutexScopedLockImpl(T& wrmtx, Locked_t)
		: m_locked(true), m_wrmtx(wrmtx) {
	}
	
	WRMutexScopedLockImpl(T& wrmtx, KeepLock_t)
		: m_locked(false), m_wrmtx(wrmtx) {
	}
	
	~WRMutexScopedLockImpl() {
		if(m_locked) {
			m_wrmtx.unlock();
			m_locked = false;
		}
	}
	
	void wrlock() {
		if(m_locked) {
			throw Mutex_error("WRMutexScopedLockImpl::wrlock error");
		}
		m_wrmtx.wrlock();
		m_locked = true;
	}

	bool tryWrlock() {
		if(m_locked) {
			return false;
		}
		m_locked = m_wrmtx.tryWrlock();
		return m_locked;
	}
	
	bool timedWrlock(const Timespec& ts) {
		if(m_locked) {
			return false;
		}
		m_locked = m_wrmtx.timedWrlock(ts);
		return m_locked;
	}

	void unlock() {
		if(!m_locked) {
			throw Mutex_error("WRMutexScopedLockImpl::unlock error");
		}
		m_wrmtx.unlock();
		m_locked = false;
	}
	
	bool getOwns() const { return m_locked; }

private:
	bool m_locked;
	T& m_wrmtx;
};

/*读写锁*/
class RWMutex : Noncopyable {
public:
	using RDLock = RDMutexScopedLockImpl<RWMutex>;	
	using WRLock = WRMutexScopedLockImpl<RWMutex>;

	RWMutex();
	~RWMutex();	
	
	void rdlock();
	bool tryrRdlock();
	bool timedRdlock(const Timespec& ts);
	
	void wrlock();
	bool tryWrlock();
	bool timedWrlock(const Timespec& ts);

	void unlock();

private:
	pthread_rwlock_t m_rwmtx;
};

/*自旋锁RAII*/
template <class T>
class SpinMutexScopedLockImpl : Noncopyable {
public:

	SpinMutexScopedLockImpl(T& spinmtx)
		: m_locked(false), m_spinmtx(spinmtx) {
		m_spinmtx.lock();
		m_locked = true;
	}

	SpinMutexScopedLockImpl(T& spinmtx, TryLock_t)
		: m_locked(false), m_spinmtx(spinmtx) {
		m_locked = m_spinmtx.trylock();
	}

	SpinMutexScopedLockImpl(T& spinmtx, Locked_t)
		: m_locked(true), m_spinmtx(spinmtx) {
	}
	
	SpinMutexScopedLockImpl(T& spinmtx, KeepLock_t)
		: m_locked(false), m_spinmtx(spinmtx) {
	}
	
	~SpinMutexScopedLockImpl() {
		if(m_locked) {
			m_spinmtx.unlock();
			m_locked = false;
		}
	}
	
	void lock() {
		if(m_locked) {
			throw Mutex_error("SpinMutexScopedLockImpl::lock error");
		}
		m_spinmtx.spinlock();
		m_locked = true;
	}

	bool trylock() {
		if(m_locked) {
			return false;
		}
		m_locked = m_spinmtx.trylock();
		return m_locked;
	}
	
	void unlock() {
		if(!m_locked) {
			throw Mutex_error("SpinMutexScopedLockImpl::unlock error");
		}
		m_spinmtx.unlock();
		m_locked = false;
	}
	
	bool getOwns() const { return m_locked; }

private:
	bool m_locked;
	T& m_spinmtx;
};

/*自旋锁*/
class SpinMutex : Noncopyable {
public:
	using Lock = SpinMutexScopedLockImpl<SpinMutex>;

	SpinMutex();
	~SpinMutex();
	
	void lock();
	bool trylock();
	void unlock();

private:
	pthread_spinlock_t m_spin;
};

/*条件变量*/
class CondVariable : Noncopyable {
public:
	
	CondVariable();
	~CondVariable();
	
	void wait(Mutex& mtx);	
	bool timedwait(Mutex& mtx, const Timespec& ts);
	void notify();
	void broadcast();

private:
	pthread_cond_t m_cond;
};

/*屏障*/
class Barrier : Noncopyable {
public:
	
	explicit Barrier(uint32_t count);
	~Barrier();
	
	void wait();
	
private:
	pthread_barrier_t m_barrier;
}; 

}	//namespace webserver

#endif // !__WEBSERVER_SYNCHRONISM_H__
