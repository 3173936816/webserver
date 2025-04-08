#ifndef __WEBSERVER_LOG_H__
#define __WEBSERVER_LOG_H__

#include <string>
#include <memory>
#include <stdint.h>
#include <sstream>
#include <stdarg.h>
#include <list>
#include <vector>
#include <fstream>
#include <unordered_map>

#include "synchronism.h"
#include "singleton.hpp"
#include "util.h"

#define WEBSERVER_LOG_LEVEL(level, logger) \
	if(logger->getLevel() <= level)	\
		webserver::LogWarp(logger, level, webserver::LogEvent::ptr(	\
			new webserver::LogEvent(time(nullptr), __FILE__, __LINE__, \
				webserver::GetThreadTid(), webserver::GetThreadName(), \
					webserver::GetCoroutineId()))).getLogEvent()->getStream()

#define WEBSERVER_LOG_DEBUG(logger)	\
	WEBSERVER_LOG_LEVEL(webserver::LogLevel::DEBUG, logger)

#define WEBSERVER_LOG_INFO(logger)	\
	WEBSERVER_LOG_LEVEL(webserver::LogLevel::INFO, logger)

#define WEBSERVER_LOG_WARN(logger)	\
	WEBSERVER_LOG_LEVEL(webserver::LogLevel::WARN, logger)

#define WEBSERVER_LOG_ERROR(logger)	\
	WEBSERVER_LOG_LEVEL(webserver::LogLevel::ERROR, logger)

#define WEBSERVER_LOG_FATAL(logger)	\
	WEBSERVER_LOG_LEVEL(webserver::LogLevel::FATAL, logger)


#define WEBSERVER_FMT_LEVEL(level, logger, str, ...) \
	if(logger->getLevel() <= level)	\
		webserver::LogWarp(logger, level, webserver::LogEvent::ptr(	\
			new webserver::LogEvent(time(nullptr), __FILE__, __LINE__, \
				webserver::GetThreadTid(), webserver::GetThreadName(), \
					webserver::GetCoroutineId()))).getLogEvent()->vspformat(str, __VA_ARGS__)

#define WEBSERVER_FMT_DEBUG(logger, str, ...)	\
	WEBSERVER_FMT_LEVEL(webserver::LogLevel::DEBUG, logger, str, __VA_ARGS__)

#define WEBSERVER_FMT_INFO(logger, str, ...)	\
	WEBSERVER_FMT_LEVEL(webserver::LogLevel::INFO, logger, str, __VA_ARGS__)

#define WEBSERVER_FMT_WARN(logger, str, ...)	\
	WEBSERVER_FMT_LEVEL(webserver::LogLevel::WARN, logger, str, __VA_ARGS__)

#define WEBSERVER_FMT_ERROR(logger, str, ...)	\
	WEBSERVER_FMT_LEVEL(webserver::LogLevel::ERROR, logger, str, __VA_ARGS__)

#define WEBSERVER_FMT_FATAL(logger, str, ...)	\
	WEBSERVER_FMT_LEVEL(webserver::LogLevel::FATAL, logger, str, __VA_ARGS__)


#define WEBSERVER_LOGMGR_ROOT()		 webserver::LoggerMgrSingleton::GetInstance()->getRoot()
#define WEBSERVER_LOGMGR_ADD(logger) webserver::LoggerMgrSingleton::GetInstance()->addLogger(logger)
#define WEBSERVER_LOGMGR_GET(owner)	 webserver::LoggerMgrSingleton::GetInstance()->getLogger(owner)
#define WEBSERVER_LOGMGR_DEL(owner)	 webserver::LoggerMgrSingleton::GetInstance()->delLogger(owner)

namespace webserver {

class Logger;

//日志级别
class LogLevel {
public:
	enum Level {
		DEBUG 		= 0,
		INFO 		= 1,
		WARN		= 2,
		ERROR		= 3,
		FATAL		= 4,
		UNKNOWN 	= 5
	};

	static std::string LevelToString(Level level, bool upper = true);
	static Level StringToLevel(const std::string& strLevel);
};

//日志事件
class LogEvent {
public:
	using ptr = std::shared_ptr<LogEvent>;
	
	LogEvent();
	LogEvent(uint64_t time, const std::string& file, uint32_t line, 
		uint32_t threadTid, const std::string& threadName, uint32_t coroutineId);
	~LogEvent() = default;

	uint64_t getTime() const { return m_time; }
	std::string getFile() const { return m_file; }
	uint32_t getLine() const { return m_line; }
	uint32_t getThreadTid() const { return m_threadTid; }
	std::string getThreadName() const { return m_threadName; }
	uint32_t getCoroutineId() const { return m_coroutineId; }
	std::ostringstream& getStream() { return m_message; }
	std::string getMessage() const { return m_message.str(); }

	void setTime(uint64_t time) { m_time = time; }
	void setFile(const std::string& file) { m_file = file; }
	void setLine(uint32_t line) { m_line = line; }
	void setThreadTid(uint32_t threadTid) { m_threadTid = threadTid; }
	void setThreadName(const std::string& threadName) { m_threadName = threadName; }
	void setCoroutineId(uint32_t coroutineId) { m_coroutineId = coroutineId; }

	void vspformat(const std::string& str, ...);
 
private:
	uint64_t m_time;				//时间戳
	std::string	m_file;				//文件名
	uint32_t m_line;				//行号
	uint32_t m_threadTid;			//线程tid
	std::string m_threadName;		//线程名称
	uint32_t m_coroutineId;			//协程id
	std::ostringstream m_message;	//日志信息
};

//格式化器
class LogFormatter {
public:
	using ptr = std::shared_ptr<LogFormatter>;
	
	explicit LogFormatter(const std::string& pattern = 
		"%D{%Y-%m-%d %H:%M:%S}%T%t%T%c%T%N%T[%L]%T[%o]%T%f:%l%T%m%n");
	~LogFormatter() = default;

	std::string format(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent);
	
	std::string getPattern() const { return m_pattern; }

public:
	class FormatPattern {
	public:
		using ptr = std::shared_ptr<FormatPattern>;
	
		FormatPattern() = default;
		virtual ~FormatPattern() = default;
		
		virtual std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) = 0;
	};

private:
	void init();

private:
	std::string m_pattern;	
	std::vector<FormatPattern::ptr> m_formatPatterns;
};

//日志输出位置
class LogAppender {
public:
	using ptr = std::shared_ptr<LogAppender>;
	using MutexType = Mutex;
	
	LogAppender();
	virtual ~LogAppender() = default;
	
	virtual void log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) = 0;
	
	void setLevel(LogLevel::Level level);
	void setFormatter(LogFormatter::ptr formatter);

	bool getOwns() const;
	LogLevel::Level getLevel() const;
	LogFormatter::ptr getFormatter() const;

protected:
	mutable MutexType m_mtx;
	bool m_owns_formatter;
	LogLevel::Level m_level;
	LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger> {
public:
	using ptr = std::shared_ptr<Logger>;	
	using MutexType = Mutex;

	explicit Logger(const std::string& owner);
	~Logger() = default;
	
	void log(LogLevel::Level level, LogEvent::ptr logEvent);
	
	//log
	void debug(LogEvent::ptr logEvent);
	void info(LogEvent::ptr logEvent);
	void warn(LogEvent::ptr logEvent);
	void error(LogEvent::ptr logEvent);
	void fatal(LogEvent::ptr logEvent);
	
	//set
	void setOwner(const std::string& owner);
	void setLevel(LogLevel::Level level);
	void setFormatter(LogFormatter::ptr formatter);
	void setAppenders(std::list<LogAppender::ptr>& appenders);
	template <typename Iterator>
	void setAppenders(Iterator beg, Iterator end) {
		MutexType::Lock locker(m_mtx);
		for(; beg != end; ++beg) {
			if(!(*beg)->getOwns()) {
				(*beg)->setFormatter(m_formatter);
			}
			m_appenders.push_back(*beg);
		}
	}
	
	//get
	std::string getOwner() const;
	LogLevel::Level getLevel() const;
	LogFormatter::ptr getFormatter() const;
	std::list<LogAppender::ptr> getAppenders() const;
	
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	void clearAppenders();

private:
	mutable MutexType m_mtx;
	std::string m_owner;
	LogLevel::Level m_level;
	LogFormatter::ptr m_formatter;
	std::list<LogAppender::ptr> m_appenders;
};

//控制台日志
class ConsoleAppender final : public LogAppender {
public:
	using ptr = std::shared_ptr<ConsoleAppender>;

	ConsoleAppender() = default;
	~ConsoleAppender() = default;
	
	void log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) override;

private:
	using MutexType = SpinMutex;
	static SpinMutex m_spin;
};

//文件日志
class FileAppender : public LogAppender {
public:
	using ptr = std::shared_ptr<FileAppender>;
	
	explicit FileAppender(const std::string& path);
	virtual ~FileAppender() = default;

	virtual void reOpen() = 0;

	std::string getFile() const;
	std::string getFileWithNoSuffix() const;
	std::string getSuffix() const;
	std::string getDirectory() const;
	std::string getPath() const;

protected:	
	void setFileWithSuffix(const std::string& file);
	void setFileWithNoSuffix(const std::string& file);

protected:
	std::string m_path;
	std::ofstream m_ofs;
};

//SoleFileAppender
class SoleFileAppender final : public FileAppender {
public:
	using ptr = std::shared_ptr<SoleFileAppender>;
	
	explicit SoleFileAppender(const std::string& path)
		: FileAppender(path) {
	}
	~SoleFileAppender() = default;

	void log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) override;
	
	void reOpen() override;
};

//DateRotateFileAppender
class DateRotateFileAppender final : public FileAppender {
public:
	using ptr = std::shared_ptr<DateRotateFileAppender>;
	
	explicit DateRotateFileAppender(const std::string& path, uint32_t maxCount = 0);
	~DateRotateFileAppender() = default;
	
	void log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) override;
	
	void reOpen() override;

private:
	std::string getDate();
	void clear();

private:
	std::string m_date;
	uint32_t m_maxCount; 
};

//SizeRotateFileAppender
class SizeRotateFileAppender final : public FileAppender {
public:
	using ptr = std::shared_ptr<SizeRotateFileAppender>;

	explicit SizeRotateFileAppender(const std::string& path, 
			const std::string& size, uint32_t maxCount = 0);
	~SizeRotateFileAppender() = default;	

	void log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) override;
	
	void reOpen() override;

private:
	off_t getSize() const;
	void clear();

private:
	std::string m_size;
	uint32_t m_maxCount;
};

//LogWarp
class LogWarp {
public:
	LogWarp(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr logEvent);
	~LogWarp();
	
	Logger::ptr getLogger() const { return m_logger; }	
	LogLevel::Level getLevel() const { return m_level; }
	LogEvent::ptr getLogEvent() const { return m_logEvent; }

private:
	Logger::ptr m_logger;
	LogLevel::Level m_level;
	LogEvent::ptr m_logEvent;
};

//LoggerManager
class LoggerManager {
public:
	using ptr = std::shared_ptr<LoggerManager>;
	using MutexType = Mutex;

	~LoggerManager() = default;

	Logger::ptr getRoot() { return m_root; }
	
	bool addLogger(Logger::ptr logger);
	Logger::ptr getLogger(const std::string& owner);
	bool delLogger(const std::string& owner);

	LoggerManager();

private:
	mutable MutexType m_mtx;
	Logger::ptr m_root;	
	std::unordered_map<std::string, Logger::ptr> m_loggers;
};

using LoggerMgrSingleton = SingletonPtr<LoggerManager>;

}	// namespace webserver

#endif //	!__WEBSERVER_LOG_H__
