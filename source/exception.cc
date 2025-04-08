#include "exception.h"

namespace webserver {

//Init_error
Init_error::Init_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Init_error::what() const noexcept { return m_errmeg.c_str(); }

//Sem_error
Sem_error::Sem_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Sem_error::what() const noexcept { return m_errmeg.c_str(); }

//Mutex_error
Mutex_error::Mutex_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Mutex_error::what() const noexcept { return m_errmeg.c_str(); }

//CondVariable_error
CondVariable_error::CondVariable_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* CondVariable_error::what() const noexcept { return m_errmeg.c_str(); }

//Barrier_error
Barrier_error::Barrier_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Barrier_error::what() const noexcept { return m_errmeg.c_str(); }

//Format_error
Format_error::Format_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Format_error::what() const noexcept { return m_errmeg.c_str(); }

//Config_error
Config_error::Config_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Config_error::what() const noexcept { return m_errmeg.c_str(); }

//LexicalCast_error
LexicalCast_error::LexicalCast_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* LexicalCast_error::what() const noexcept { return m_errmeg.c_str(); }

//Thread_error
Thread_error::Thread_error(const std::string& errmeg) : m_errmeg(errmeg) {}
const char* Thread_error::what() const noexcept { return m_errmeg.c_str(); }

}	//namespace webserver
