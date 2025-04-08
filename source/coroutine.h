#ifndef __WEBSERVER_COROUTINE_H__
#define __WEBSERVER_COROUTINE_H__

#include <memory>
#include <string>
#include <functional>
#include <ucontext.h>

#include "noncopyable.h"

namespace webserver {

class Coroutine : Noncopyable {
	friend class now_coroutine;
public:
	using ptr = std::shared_ptr<Coroutine>;
	
	enum State {
		INIT 	= 0,
		EXEC	= 1,
		TERM	= 3,
		HOLD	= 4,
		EXCEPT	= 5
	};	
	
	explicit Coroutine(std::function<void()> func);
	~Coroutine();

	void swapIn();
	void reset(std::function<void()> func = nullptr);

	uint32_t getCid() const { return m_cid; };
	State getState() const { return m_state; }

private:
	Coroutine();

private:
	static void cor_routine();

private:
	thread_local static Coroutine::ptr m_mainCoroutine;
	thread_local static Coroutine* m_currCoroutine;

private:
	uint32_t m_cid;
	State m_state;
	void* m_stack;
	uint32_t m_size;
	ucontext_t m_ctx;
	std::function<void()> m_func;
};

// using as a namespace
class now_coroutine : Noncopyable {
	friend class Coroutine;
public:
	
	static void SwapOut();
	
	static uint64_t GetCid();
	static Coroutine::State GetState();

private:
	now_coroutine() = delete;
	~now_coroutine() = delete;
};

}	// namespace webserver

#endif // !__WEBSERVER_COROUTINE_H__
