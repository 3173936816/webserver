#ifndef __WEBSERVER_EXCPETION_H__
#define __WEBSERVER_EXCPETION_H__

#include <exception>
#include <string>

namespace webserver {

//Init
class Init_error : public std::exception {
public:
	Init_error(const std::string& errmeg);
	~Init_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Sem
class Sem_error : public std::exception {
public:
	Sem_error(const std::string& errmeg);
	~Sem_error() = default;

	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Mutex
class Mutex_error : public std::exception {
public:
	Mutex_error(const std::string& errmeg);
	~Mutex_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//CondVariable
class CondVariable_error : public std::exception {
public:
	CondVariable_error(const std::string& errmeg);
	~CondVariable_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Barrier
class Barrier_error : public std::exception {
public:
	Barrier_error(const std::string& errmeg);
	~Barrier_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Format
class Format_error : public std::exception {
public:
	Format_error(const std::string& errmeg);
	~Format_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Config
class Config_error : public std::exception {
public:
	Config_error(const std::string& errmeg);
	~Config_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//LexicalCast
class LexicalCast_error : public std::exception {
public:
	LexicalCast_error(const std::string& errmeg);
	~LexicalCast_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

//Thread
class Thread_error : public std::exception {
public:
	Thread_error(const std::string& errmeg);
	~Thread_error() = default;
	
	const char* what() const noexcept override;
private:
	std::string m_errmeg;
};

}	//namespace webserver

#endif // !__WEBSERVER_EXCPETION_H__
