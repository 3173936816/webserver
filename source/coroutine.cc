#include "coroutine.h"
#include "config.h"
#include "util.h"
#include "log.h"
#include "macro.h"
#include "synchronism.h"

namespace webserver {

static constexpr const char* s_logger = "system";

//coroutine configuration integration
namespace {

static ConfigVar<Map<std::string, std::string> >::ptr Coroutine_Config_Var = 
	ConfigSingleton::GetInstance()->addConfigVar<Map<std::string, std::string> >("coroutine", 
		Map<std::string, std::string>{{"stackSize", "1024 * 1024"}}, 
			"this configvar defined coroutine stack size");

static std::string g_stack_size{};

struct CoroutineConfig_Initer {
	using Type = Map<std::string, std::string>;
	
	CoroutineConfig_Initer() {
		g_stack_size = Coroutine_Config_Var->getVariable().at("stackSize");

		Coroutine_Config_Var->addMonitor("coroutine_01", [](const Type& oldVar, const Type& newVar) {
			g_stack_size = newVar.at("stackSize");
		});
	}
};

static CoroutineConfig_Initer CoroutineConfig_Initer_CXX_01;

}

thread_local Coroutine::ptr Coroutine::m_mainCoroutine{nullptr};
thread_local Coroutine* Coroutine::m_currCoroutine{nullptr};

Coroutine::Coroutine(std::function<void()> func)
	: m_cid(GetUUID()), m_state(EXCEPT)
	, m_stack(nullptr), m_size(0), m_ctx{}
	, m_func(func) {
	
	m_size = StringCalculate(g_stack_size);
	m_stack = malloc(m_size * sizeof(char));
	
	int32_t rt = getcontext(&m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "Coroutine::Coroutine --- getcontext error");
	
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_flags = 0;
	m_ctx.uc_stack.ss_size = m_size;

	makecontext(&m_ctx, &Coroutine::cor_routine, 0);
	
	m_state = INIT;
}

Coroutine::Coroutine()
	: m_cid(GetThreadTid()), m_state(EXEC)
	, m_stack(nullptr), m_size(0), m_ctx{}
	, m_func{} {
	
	int32_t rt = getcontext(&m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "Coroutine::Coroutine2 --- getcontext error");
}

Coroutine::~Coroutine() {
	if(m_mainCoroutine.get() != this) {
		WEBSERVER_ASSERT(m_state == TERM
			|| m_state == INIT);
		if(m_stack != nullptr) {
			free(m_stack);
		}
	}
}

void Coroutine::swapIn() {
	if(m_mainCoroutine == nullptr) {
		m_mainCoroutine.reset(new Coroutine());
	}
	WEBSERVER_ASSERT(m_mainCoroutine != nullptr
		&& (m_state == INIT || m_state == HOLD));

	m_currCoroutine = this;
	m_state = EXEC;
	
	int32_t rt = swapcontext(&m_mainCoroutine->m_ctx, &m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "Coroutine::swapIn --- swapcontext error");
}

void Coroutine::reset(std::function<void()> func) {
	int32_t rt = getcontext(&m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "Coroutine::reset --- getcontext error");
	
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_flags = 0;
	m_ctx.uc_stack.ss_size = m_size;
	
	m_state = INIT;
	if(func) {
		m_func = func;
	}

	makecontext(&m_ctx, &Coroutine::cor_routine, 0);
}

void Coroutine::cor_routine() {
	WEBSERVER_ASSERT(m_mainCoroutine != nullptr
		&& m_currCoroutine != nullptr
		&& m_currCoroutine != m_mainCoroutine.get()
		&& m_currCoroutine->m_state == EXEC);

	try {
		m_currCoroutine->m_func();
	} catch(std::exception& e) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_GET(s_logger))
			<< "Coroutine::cor_routine func error --- what : " 
			<< e.what() << "  cid : " << m_currCoroutine->m_cid;

		m_currCoroutine->m_state = EXCEPT;
		return;
	}

	Coroutine* cor = Coroutine::m_currCoroutine;
	cor->m_state = TERM;
	Coroutine::m_currCoroutine = Coroutine::m_mainCoroutine.get();

	int32_t rt = swapcontext(&cor->m_ctx, &m_mainCoroutine->m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "Coroutine::cor_routine --- swapcontext error");
}

void now_coroutine::SwapOut() {
	WEBSERVER_ASSERT(Coroutine::m_mainCoroutine != nullptr
		&& Coroutine::m_currCoroutine != Coroutine::m_mainCoroutine.get()
		&& Coroutine::m_currCoroutine->m_state == Coroutine::EXEC);

	Coroutine* cor = Coroutine::m_currCoroutine;
	cor->m_state = Coroutine::HOLD;
	Coroutine::m_currCoroutine = Coroutine::m_mainCoroutine.get();
	
	int32_t rt = swapcontext(&cor->m_ctx, &Coroutine::m_mainCoroutine->m_ctx);
	WEBSERVER_ASSERT2(rt == 0, "now_coroutine::swapOut --- swapcontext error");
}

uint64_t now_coroutine::GetCid() {
	if(Coroutine::m_currCoroutine == nullptr) {
		return GetThreadTid();
	}
	return Coroutine::m_currCoroutine->m_cid;
}

Coroutine::State now_coroutine::GetState() {
	if(Coroutine::m_currCoroutine == nullptr) {
		return Coroutine::EXEC;
	}
	return Coroutine::m_currCoroutine->m_state;
}

}	// namespace webserver
