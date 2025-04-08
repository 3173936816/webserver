#include "synchronism.h"
#include "macro.h"

#include <iostream>
#include <string.h>

namespace webserver {

Semaphore::Semaphore(uint32_t count) 
	: m_sem{} {
	int32_t rt = sem_init(&m_sem, 0, count);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Semaphore::Semaphore error");
		throw Init_error("sem_init");
	}
}

Semaphore::~Semaphore() {
	int32_t rt = sem_destroy(&m_sem);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "Semaphore::~Semaphore error");
	}
}

void Semaphore::notify() {
	int32_t rt = sem_post(&m_sem);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Semaphore::notify error");
		throw Sem_error("sem_post");
	}
}

void Semaphore::wait() {
	int32_t rt = sem_wait(&m_sem);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Semaphore::wait error");
		throw Sem_error("sem_wait");
	}
}

bool Semaphore::trywait() {
	return sem_trywait(&m_sem) == 0;
}

bool Semaphore::timedwait(const Timespec& ts) {
	return sem_timedwait(&m_sem, ts.get()) == 0;
}

int32_t Semaphore::getValue() {
	int32_t semCount = -1;
	int32_t rt = sem_getvalue(&m_sem, &semCount);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Semaphore::getValue error");
		return -1;
	}
	return semCount;
}

Mutex::Mutex(Attr attr) 
	: m_attr(attr)
	, m_mtx{} {
	pthread_mutexattr_t mutexattr;
	int32_t rt = pthread_mutexattr_init(&mutexattr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::Mutex error");
		throw Init_error("pthread_mutexattr_init");
	}
	
	rt = pthread_mutexattr_settype(&mutexattr, attr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::Mutex error");
		throw Init_error("pthread_mutexattr_settype");
	}
	
	rt = pthread_mutex_init(&m_mtx, &mutexattr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::Mutex error");
		throw Init_error("pthread_mutex_init");
	}
	
	rt = pthread_mutexattr_destroy(&mutexattr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::Mutex error");
		throw Init_error("pthread_mutexattr_destroy");
	}
}

Mutex::~Mutex() {
	int32_t rt = pthread_mutex_destroy(&m_mtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt,"Mutex::~Mutex error");
	}
}

void Mutex::lock() {
	int32_t rt = pthread_mutex_lock(&m_mtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::lock error");
		throw Mutex_error("pthread_mutex_lock");
	}
}

bool Mutex::trylock() {
	return pthread_mutex_trylock(&m_mtx) == 0;
}

bool Mutex::timedlock(const Timespec& ts) {
	return pthread_mutex_timedlock(&m_mtx, ts.get()) == 0;
}

void Mutex::unlock() {
	int32_t rt = pthread_mutex_unlock(&m_mtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Mutex::unlock error");
		throw Mutex_error("pthread_mutex_unlock");
	}
}

RWMutex::RWMutex() 
	: m_rwmtx{} {
	int32_t rt = pthread_rwlock_init(&m_rwmtx, nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "RWMutex::RWMutex error");
		throw Init_error("pthread_rwlock_init");
	}
}

RWMutex::~RWMutex() {
	int32_t rt = pthread_rwlock_destroy(&m_rwmtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "RWMutex::~RWMutex");
	}
}

void RWMutex::rdlock() {
	int32_t rt = pthread_rwlock_rdlock(&m_rwmtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "RWMutex::rdlock error");
		throw Mutex_error("pthread_rwlock_rdlock");
	}
}

bool RWMutex::tryrRdlock() {
	return pthread_rwlock_tryrdlock(&m_rwmtx) == 0;
}

bool RWMutex::timedRdlock(const Timespec& ts) {
	return pthread_rwlock_timedrdlock(&m_rwmtx, ts.get()) == 0;
}

void RWMutex::wrlock() {
	int32_t rt = pthread_rwlock_wrlock(&m_rwmtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "RWMutex::wrlock error");
		throw Mutex_error("pthread_rwlock_wrlock");
	}
}

bool RWMutex::tryWrlock() {
	return pthread_rwlock_trywrlock(&m_rwmtx) == 0;
}

bool RWMutex::timedWrlock(const Timespec& ts) {
	return pthread_rwlock_timedwrlock(&m_rwmtx, ts.get()) == 0;
}

void RWMutex::unlock() {
	int32_t rt = pthread_rwlock_unlock(&m_rwmtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "RWMutex::unlock error");
		throw Mutex_error("pthread_rwlock_unlock");
	}
}

SpinMutex::SpinMutex()
	: m_spin{} {
	int32_t rt = pthread_spin_init(&m_spin, 0);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "SpinMutex::SpinMutex error");
		throw Init_error("pthread_spin_init");
	}
}

SpinMutex::~SpinMutex() {
	int32_t rt = pthread_spin_destroy(&m_spin);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "SpinMutex::~SpinMutex");
	}
}

void SpinMutex::lock() {
	int32_t rt = pthread_spin_lock(&m_spin);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "SpinMutex::lock error");
		throw Mutex_error("pthread_spin_lock");
	}
}

bool SpinMutex::trylock() {
	return pthread_spin_trylock(&m_spin) == 0;
}

void SpinMutex::unlock() {
	int32_t rt = pthread_spin_unlock(&m_spin);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "SpinMutex::unlock error");
		throw Mutex_error("pthread_spin_lock");
	}
} 

CondVariable::CondVariable() 
	: m_cond{} {
	int32_t rt = pthread_cond_init(&m_cond, nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "CondVariable::CondVariable error");
		throw Init_error("pthread_cond_init");
	}
}

CondVariable::~CondVariable() {
	int32_t rt = pthread_cond_destroy(&m_cond);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "CondVariable::~CondVariable");
	}
}

void CondVariable::wait(Mutex& mtx) {
	if(mtx.getAttr() == Mutex::RECURSIVE) {
		throw CondVariable_error("Mutex attr can't be recursive");
	}	

	int32_t rt = pthread_cond_wait(&m_cond, &mtx.m_mtx);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "CondVariable::wait error");
		throw CondVariable_error("pthread_cond_wait");
	}
}

bool CondVariable::timedwait(Mutex& mtx, const Timespec& ts) {
	if(mtx.getAttr() == Mutex::RECURSIVE) {
		throw CondVariable_error("Mutex attr can't be recursive");
	}	
	return pthread_cond_timedwait(&m_cond, &mtx.m_mtx, ts.get()) == 0;
}

void CondVariable::notify() {
	int32_t rt = pthread_cond_signal(&m_cond);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "CondVariable::notify error");
		throw CondVariable_error("pthread_cond_signal");
	}
}

void CondVariable::broadcast() {
	int32_t rt = pthread_cond_broadcast(&m_cond);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "CondVariable::broadcast error");
		throw CondVariable_error("pthread_cond_broadcast");
	}
}

Barrier::Barrier(uint32_t count) 
	: m_barrier{} {
	int32_t rt = pthread_barrier_init(&m_barrier, nullptr, count);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Barrier::Barrier error");
		throw Init_error("pthread_barrier_init");
	}
}

Barrier::~Barrier() {
	int32_t rt = pthread_barrier_destroy(&m_barrier);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "Barrier::~Barrier");
	}
}

void Barrier::wait() {
	int32_t rt = pthread_barrier_wait(&m_barrier);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_STD_LOG(rt, "Barrier::wait error");
		throw Barrier_error("pthread_barrier_wait");
	}
}

}	//namespace webserver
