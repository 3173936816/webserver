#include "log.h"
#include "macro.h"
#include "util.h"
#include "config.h"

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>
#include <functional>

namespace webserver {

//LogLevel
std::string LogLevel::LevelToString(Level level, bool upper) {
#define XX(l, r) 		\
	case l: {			\
		return #r;		\
		break;			\
	}	

	if(WEBSERVER_LIKELY(upper)) {
		switch(level) {
			XX(DEBUG, DEBUG)
			XX(INFO, INFO)
			XX(WARN, WARN)
			XX(ERROR, ERROR)
			XX(FATAL, FATAL)
		default:
			return "UNKNOWN";
		}
	} else {
		switch(level) {
			XX(DEBUG, debug)
			XX(INFO, info)
			XX(WARN, warn)
			XX(ERROR, error)
			XX(FATAL, fatal)
		default:
			return "unknown";
		}
	}
#undef XX
}

LogLevel::Level LogLevel::StringToLevel(const std::string& strLevel) {
#define XX(comp, r)				\
	if(strLevel == #comp) {		\
		return r;				\
	}
	
	XX(DEBUG, DEBUG)
	XX(INFO, INFO)
	XX(WARN, WARN)
	XX(ERROR, ERROR)
	XX(FATAL, FATAL)

	XX(debug, DEBUG)
	XX(info, INFO)
	XX(warn, WARN)
	XX(error, ERROR)
	XX(fatal, FATAL)
#undef XX
	return UNKNOWN;	
}

//LogEvent
LogEvent::LogEvent() 
	: m_time(0), m_file{}, m_line(0), m_threadTid(0)
	, m_threadName{}, m_coroutineId(0), m_message{} {
}

LogEvent::LogEvent(uint64_t time, const std::string& file, uint32_t line, 
		uint32_t threadTid, const std::string& threadName, uint32_t coroutineId) 
	: m_time(time), m_file(file), m_line(line), m_threadTid(threadTid)
	, m_threadName(threadName), m_coroutineId(coroutineId), m_message{} {
}

void LogEvent::vspformat(const std::string& str, ...) {
	va_list va;
	va_start(va, str);

	char buff[1024] = {0};
	int32_t rt = vsprintf(buff, str.c_str(), va);
	if(WEBSERVER_UNLIKELY(rt <= 0)) {
		LIBFUN_ABORT_LOG(rt, "LogEvent::vspformat--vsprintf error");
	}
	getStream() << buff;
	va_end(va); 
}

//LogFormatter
/*
%o owner			-日志归属用户 		%L Level			-日志级别
%f file				-文件名称			%l line				-行号
%t Tid				-线程tid			%N Name				-线程名称
%c corottineId		-协程id				%D Time				-时间
%m message			-日志信息			%s string			-自定义信息
%T \t				-制表符				%n std::endl		-换行
*/

class OwnerFormatPattern : public LogFormatter::FormatPattern {
public:
	OwnerFormatPattern(const std::string&) {}
	~OwnerFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logger->getOwner();
		return oss;
	}
};

class LevelFormatPattern : public LogFormatter::FormatPattern {
public:
	LevelFormatPattern(const std::string&) {}
	~LevelFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << LogLevel::LevelToString(level);
		return oss;
	}
};

class FileFormatPattern : public LogFormatter::FormatPattern {
public:
	FileFormatPattern(const std::string&) {}
	~FileFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getFile();
		return oss;
	}
};

class LineFormatPattern : public LogFormatter::FormatPattern {
public:
	LineFormatPattern(const std::string&) {} 
	~LineFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getLine();
		return oss;
	}
};

class ThreadTidFormatPattern : public LogFormatter::FormatPattern {
public:
	ThreadTidFormatPattern(const std::string&) {} 
	~ThreadTidFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getThreadTid();
		return oss;
	}
};

class ThreadNameFormatPattern : public LogFormatter::FormatPattern {
public:
	ThreadNameFormatPattern(const std::string&) {} 
	~ThreadNameFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getThreadName();
		return oss;
	}
};

class CoroutineIdFormatPattern : public LogFormatter::FormatPattern {
public:
	CoroutineIdFormatPattern(const std::string&) {}
	~CoroutineIdFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getCoroutineId();
		return oss;
	}
};

class DateFormatPattern : public LogFormatter::FormatPattern {
public:
	DateFormatPattern(const std::string& format = "%Y-%m-%d %H-%M-%S")
		: m_format(format) {
	}
	~DateFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		time_t t = static_cast<time_t>(logEvent->getTime());
		struct tm* tmp = localtime(&t); 
		if(WEBSERVER_UNLIKELY(!tmp)) {
			LIBFUN_STD_LOG(errno, "DateFormattern::format--localtime error");
			oss << "xx-xx-xx";
			return oss;
		}
		
		char buff[56] = {0};	
		int32_t rt = ::strftime(buff, sizeof(buff), m_format.c_str(), tmp);
		if(WEBSERVER_UNLIKELY(!rt)) {
			LIBFUN_STD_LOG(errno, "DateFormattern::format--strftime error");
			oss << "xx-xx-xx";
			return oss;
		}
		
		oss << buff;
		return oss;
	}

private:
	std::string m_format;
};

class MessageFormatPattern : public LogFormatter::FormatPattern {
public:
	MessageFormatPattern(const std::string&) {} 
	~MessageFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << logEvent->getMessage();
		return oss;
	}
};

class StringFormatPattern : public LogFormatter::FormatPattern {
public:
	StringFormatPattern(const std::string& str)
		: m_str(str) {
	}
	~StringFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << m_str;
		return oss;
	}

private:
	std::string m_str;
};

class TabFormatPattern : public LogFormatter::FormatPattern {
public:
	TabFormatPattern(const std::string&) {}
	~TabFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << '\t';
		return oss;
	}
};

class NewLineFormatPattern : public LogFormatter::FormatPattern {
public:
	NewLineFormatPattern(const std::string&) {} 
	~NewLineFormatPattern() = default;
	
	std::ostringstream& format(std::ostringstream& oss, 
			std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr logEvent) override {
		oss << std::endl;
		return oss;
	}
};

LogFormatter::LogFormatter(const std::string& pattern)
	: m_pattern(pattern), m_formatPatterns{} {
	init();
}
	
std::string LogFormatter::format(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) {
	std::ostringstream oss;
	for(auto& formatPattern: m_formatPatterns) {
		formatPattern->format(oss, logger, level, logEvent);
	}
	return oss.str();
}

void LogFormatter::init() {
	std::vector<std::pair<int32_t, std::string> > vec_pair;
	bool percentSign = false;
	for(size_t i = 0; i < m_pattern.size(); ++i) {
		if(m_pattern[i] == '%') {
			if(percentSign) {
				//std::cerr << 1 << std::endl;
				throw Format_error("fmt error");
			}
			percentSign = true;
			continue;
		} else if(m_pattern[i] == '{') {
			if(percentSign || (i && m_pattern[i - 1] != 'D')) {
				//std::cerr << 2 << std::endl;
				throw Format_error("fmt error");	
			} 
			size_t j = i + 1;
			for(; j < m_pattern.size(); ++j) {
				if(m_pattern[j] == '}') {
					break;
				}
			}
			if(j < m_pattern.size()) {
				std::string tmp = m_pattern.substr(i + 1, j - i - 1);
				vec_pair.push_back(std::make_pair(2, tmp));
				i = j;
			} else {
				//std::cerr << 3 << std::endl;
				throw Format_error("fmt error");		
			}
		} else {
			if(percentSign) {
				vec_pair.push_back(std::make_pair(1, std::string(1, m_pattern[i])));
			} else {
				vec_pair.push_back(std::make_pair(3, std::string(1, m_pattern[i])));	
			}
			percentSign = false;
		}
	}

	static std::unordered_map<std::string, 
		std::function<LogFormatter::FormatPattern::ptr(const std::string& str)> > Item_Map = {
#define XX(index, type)																		\
		{ #index, [=](const std::string& str)->LogFormatter::FormatPattern::ptr { 			\
					return LogFormatter::FormatPattern::ptr(new type##FormatPattern(str));	\
			      }																			\
		}																					\

		XX(o, Owner),
		XX(L, Level),
		XX(f, File),
		XX(l, Line),
		XX(t, ThreadTid),
		XX(N, ThreadName),
		XX(c, CoroutineId),
		XX(D, Date),
		XX(m, Message),
		XX(s, String),
		XX(T, Tab),
		XX(n, NewLine)
#undef XX
	};
	
	for(size_t i = 0; i < vec_pair.size(); ++i) {
		if(vec_pair[i].first == 1) {
			auto it = Item_Map.find(vec_pair[i].second);
			if(it == Item_Map.end()) {
				m_formatPatterns.emplace_back(new StringFormatPattern(" <<error_fmt>> "));
				continue;
			}
			if(vec_pair[i].second == "D") {
				if(i + 1 < vec_pair.size() && vec_pair[i + 1].first == 2) {
					m_formatPatterns.push_back(it->second(vec_pair[i + 1].second));
					i += 1;
					continue;
				} else {
					m_formatPatterns.push_back(it->second("%Y-%m-%d %H:%M:%S"));	
				}
			} else {
				m_formatPatterns.push_back(it->second(""));	
			}
		} else if(vec_pair[i].first == 3) {
			m_formatPatterns.emplace_back(new StringFormatPattern(vec_pair[i].second));
		}
	}
}

//LogAppender
LogAppender::LogAppender() 
	: m_mtx{}, m_owns_formatter(false)
	, m_level(LogLevel::DEBUG), m_formatter{} {
}

void LogAppender::setLevel(LogLevel::Level level) {
	MutexType::Lock locker(m_mtx);
	m_level = level;
}

void LogAppender::setFormatter(LogFormatter::ptr formatter) {
	MutexType::Lock locker(m_mtx);
	m_formatter = formatter;
	m_owns_formatter = true;
}

bool LogAppender::getOwns() const {
	MutexType::Lock locker(m_mtx);
	return m_owns_formatter;
}

LogLevel::Level LogAppender::getLevel() const {
	MutexType::Lock locker(m_mtx);
	return m_level;
}

LogFormatter::ptr LogAppender::getFormatter() const {
	MutexType::Lock locker(m_mtx);
	return m_formatter;	
}

//Logger
Logger::Logger(const std::string& owner) 
	:m_mtx{/*Mutex::CHECK*/Mutex::RECURSIVE}, m_owner(owner)
	, m_level(LogLevel::DEBUG)
	, m_formatter{new LogFormatter()}, m_appenders{} {
}

void Logger::log(LogLevel::Level level, LogEvent::ptr logEvent) {
	MutexType::Lock locker(m_mtx);
	if(level >= m_level) {
		Logger::ptr self = shared_from_this();
		for(auto& appender : m_appenders) {
			appender->log(self, level, logEvent);
		}
	}
}

void Logger::debug(LogEvent::ptr logEvent) {
	log(LogLevel::DEBUG, logEvent);
}

void Logger::info(LogEvent::ptr logEvent) {
	log(LogLevel::INFO, logEvent);
}

void Logger::warn(LogEvent::ptr logEvent) {
	log(LogLevel::WARN, logEvent);
}

void Logger::error(LogEvent::ptr logEvent) {
	log(LogLevel::ERROR, logEvent);
}

void Logger::fatal(LogEvent::ptr logEvent) {
	log(LogLevel::FATAL, logEvent);
}

void Logger::setOwner(const std::string& owner) {
	MutexType::Lock locker(m_mtx);
	m_owner = owner;
}

void Logger::setLevel(LogLevel::Level level) {
	MutexType::Lock locker(m_mtx);
	m_level = level;
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
	MutexType::Lock locker(m_mtx);
	m_formatter = formatter;
}

void Logger::setAppenders(std::list<LogAppender::ptr>& appenders) {
	using std::swap;
	MutexType::Lock locker(m_mtx);
	for(auto& appender : appenders) {
		if(!appender->getOwns()) {
			appender->setFormatter(m_formatter);
		}
	}
	swap(m_appenders, appenders);
}

std::string Logger::getOwner() const {
	MutexType::Lock locker(m_mtx);
	return m_owner;
}

LogLevel::Level Logger::getLevel() const {
	MutexType::Lock locker(m_mtx);
	return m_level;
}

LogFormatter::ptr Logger::getFormatter() const {
	MutexType::Lock locker(m_mtx);
	return m_formatter;
}

std::list<LogAppender::ptr> Logger::getAppenders() const {
	MutexType::Lock locker(m_mtx);
	return m_appenders;
}

void Logger::addAppender(LogAppender::ptr appender) {
	MutexType::Lock locker(m_mtx);
	if(!appender->getOwns()) {
		appender->setFormatter(m_formatter);
	}
	m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	MutexType::Lock locker(m_mtx);
	auto it = m_appenders.begin();
	for(; it != m_appenders.end(); ++it) {
		if((*it).get() == appender.get()) {
			break;
		}
	}	

	if(it != m_appenders.end()) {
		m_appenders.erase(it);
	}
}

void Logger::clearAppenders() {
	MutexType::Lock locker(m_mtx);
	m_appenders.clear();
}

//ConsoleAppender
SpinMutex ConsoleAppender::m_spin;

void ConsoleAppender::log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) {
	MutexType::Lock locker1(m_spin);
	LogAppender::MutexType::Lock locker2(m_mtx);
	if(level >= m_level) {
		std::clog << m_formatter->format(logger, level, logEvent);
		//std::clog << logger->getOwner() << std::endl;
	}
}

//FileAppender

FileAppender::FileAppender(const std::string& path)
	: m_path(path), m_ofs{} {
	//TODO handle path

	size_t pos = m_path.rfind('.');
	if(pos == std::string::npos 
		|| m_path[pos + 1] == '/') {
		m_path += ".log";
	}
}

std::string FileAppender::getFile() const {
	size_t pos = m_path.rfind('/');
	return m_path.substr(pos + 1);
}

std::string FileAppender::getFileWithNoSuffix() const {
	std::string file = getFile();
	return file.substr(0, file.rfind('.'));
}

std::string FileAppender::getSuffix() const {
	std::string file = getFile();
	size_t pos = file.rfind('.');
	return file.substr(pos);
}

std::string FileAppender::getDirectory() const {
	size_t pos = m_path.rfind('/');
	return m_path.substr(0, pos + 1);
}

std::string FileAppender::getPath() const {
	return m_path;
}

void FileAppender::setFileWithSuffix(const std::string& file) {
	size_t pos = m_path.rfind('/');
	m_path.replace(pos + 1, std::string::npos, file);
}

void FileAppender::setFileWithNoSuffix(const std::string& file) {
	size_t pos = m_path.rfind('/');
	size_t pos1 = m_path.rfind('.');
	m_path.replace(pos + 1, pos1 - pos - 1, file);
}

//SoleFileAppender
void SoleFileAppender::log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) {
	LogAppender::MutexType::Lock locker(m_mtx);
	if(level >= m_level) {
		reOpen();
		m_ofs << m_formatter->format(logger, level, logEvent);
	}
}
	
void SoleFileAppender::reOpen() {
	if(m_ofs.is_open()) {
		m_ofs.close();
	}
	m_ofs.open(m_path, std::ios_base::app);
	if(!m_ofs.is_open()) {
		LIBFUN_STD_LOG(errno, "SoleFileAppender::reOpen error");
		return;
	}
}

//DateRotateFileAppender
DateRotateFileAppender::DateRotateFileAppender(const std::string& path, uint32_t maxCount)
	: FileAppender(path), m_date{}, m_maxCount(maxCount) {
	setFileWithNoSuffix(getFileWithNoSuffix() + "_" + GetDate());
	clear();
}

void DateRotateFileAppender::log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) {
	LogAppender::MutexType::Lock locker(m_mtx);
	if(level >= m_level) {
		reOpen();
		m_ofs << m_formatter->format(logger, level, logEvent);
	}
} 
	
void DateRotateFileAppender::reOpen() {
	if(m_ofs.is_open()) {
		m_ofs.close();
	}
	std::string nowDate = GetDate();
	if(nowDate != getDate()) {
		clear();
		std::string file = getFileWithNoSuffix();
		size_t pos = file.rfind('_');
		setFileWithNoSuffix(file.substr(0, pos) + "_" + GetDate());
	}
	m_ofs.open(m_path, std::ios_base::app);
	if(!m_ofs.is_open()) {
		LIBFUN_ABORT_LOG(errno, "DateFileAppender::reOpen error");
	}
} 

std::string DateRotateFileAppender::getDate() {
	std::string file = getFile();
	size_t pos1 = file.rfind('_');
	size_t pos2 = file.rfind('.');
	return file.substr(pos1 + 1, pos2 - pos1 + 1);
}

void DateRotateFileAppender::clear() {
	if(!m_maxCount) {
		return;
	}
	
	DirentList dirents;
	LogFileSelect(getDirectory(), getSuffix(), dirents, Date_lower_compar);
	
	if(dirents.size() < m_maxCount)	{
		return;
	}
	
	size_t unlink_count = getFile() == dirents.back()->d_name
		 	? dirents.size() - m_maxCount
			: dirents.size() - m_maxCount + 1;
	for(size_t i = 0; i < unlink_count; ++i) {
		std::string unlink_file_path = getDirectory() + dirents[i]->d_name;
		unlink(unlink_file_path.c_str());
	}
}

//SizeRotateFileAppender
SizeRotateFileAppender::SizeRotateFileAppender(const std::string& path, 
			const std::string& size, uint32_t maxCount)
		: FileAppender(path), m_size(size), m_maxCount(maxCount) {
	clear();
	DirentList dirents;
	LogFileSelect(getDirectory(), getSuffix(), dirents, Size_lower_compar);
	if(dirents.empty()) {
		setFileWithNoSuffix(getFileWithNoSuffix() + "_" + std::to_string(1));
	} else {
		setFileWithNoSuffix(getFileWithNoSuffix() + "_" + std::to_string(dirents.size()));
	}
}

void SizeRotateFileAppender::log(std::shared_ptr<Logger> logger, 
		LogLevel::Level level, LogEvent::ptr logEvent) {
	LogAppender::MutexType::Lock locker(m_mtx);
	if(level >= m_level) {
		reOpen();
		m_ofs << m_formatter->format(logger, level, logEvent);
	}
}
	
void SizeRotateFileAppender::reOpen() {
	if(m_ofs.is_open()) {
		m_ofs.close();
	}

	off_t now_size;
	try {
		now_size = GetFileSize(getPath());
	} catch(...) {
		now_size = 0;
	}

	if(now_size >= (off_t)StringCalculate(m_size)) {
		clear();
		std::string file_name = getFile();
		size_t pos1 = file_name.rfind('_');
		size_t pos2 = file_name.rfind('.');
		uint32_t num = std::atoi(file_name.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
		if(!m_maxCount || num < m_maxCount) {
			file_name.replace(pos1 + 1, pos2 - pos1 - 1, std::to_string(num + 1));
			setFileWithSuffix(file_name);
		}
	}
	m_ofs.open(m_path, std::ios_base::app);
	if(!m_ofs.is_open()) {
		LIBFUN_ABORT_LOG(errno, "SizeRotateFileAppender::reOpen error");
	}
}

void SizeRotateFileAppender::clear() {
	if(!m_maxCount) {
		return;
	}
	
	DirentList dirents;
	LogFileSelect(getDirectory(), getSuffix(), dirents, Size_lower_compar);
	
	if(dirents.size() < m_maxCount) {
		return;
	}

	std::string last_file_path = getDirectory() + dirents.back()->d_name;
	off_t file_size = GetFileSize(last_file_path);

	size_t unlink_count = file_size < (off_t)StringCalculate(m_size) 
			? (dirents.size() - m_maxCount) 
			: (dirents.size() - m_maxCount + 1);
	
	for(size_t i = 0; i < dirents.size(); ++i) {
		if(i < unlink_count) {
			std::string unlink_file_path = getDirectory() + dirents[i]->d_name;
			unlink(unlink_file_path.c_str());
		} else {
			std::string file_name = dirents[i]->d_name;
			std::string new_file_name = file_name;
			size_t pos1 = new_file_name.rfind('_');
			size_t pos2 = new_file_name.rfind('.');
			uint32_t num = std::atoi(new_file_name.substr(
					pos1 + 1 , pos2 - pos1 - 1).c_str());
			new_file_name.replace(pos1 + 1, pos2 - pos1 - 1, std::to_string(num - 1));
			
			std::string old_path = getDirectory() + file_name;
			std::string new_path = getDirectory() + new_file_name;
			rename(old_path.c_str(), new_path.c_str());
		}
	}
}

//LogWarp
LogWarp::LogWarp(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr logEvent) 
	: m_logger(logger), m_level(level), m_logEvent(logEvent) {	
}

LogWarp::~LogWarp() {
	m_logger->log(m_level, m_logEvent);
}

//LoggerManager
LoggerManager::LoggerManager()
	: m_mtx{}
	, m_root(new Logger("root"))
	, m_loggers{} {
	ConsoleAppender::ptr console(new ConsoleAppender());
	m_root->addAppender(console);
} 

bool LoggerManager::addLogger(Logger::ptr logger) {
	std::string owner = logger->getOwner();
	if(owner == "root") {
		return false;
	}
	MutexType::Lock locker(m_mtx);
	auto it = m_loggers.find(owner);
	if(it != m_loggers.end()) {
		return false;
	}
	m_loggers[owner] = logger;
	return true;
}

Logger::ptr LoggerManager::getLogger(const std::string& owner) {
	MutexType::Lock locker(m_mtx);
	auto it = m_loggers.find(owner);
	if(it != m_loggers.end()) {
		return it->second;
	}
	
	return m_root;
}

bool LoggerManager::delLogger(const std::string& owner) {
	MutexType::Lock locker(m_mtx);
	auto it = m_loggers.find(owner);
	if(it != m_loggers.end()) {
		m_loggers.erase(it);
		return true;
	}
	return false;
}


//Log configuration integration

//ConsoleLog
struct ConsoleLogDefine {
	LogLevel::Level level;
	std::string format;
	
	bool operator<(const ConsoleLogDefine& rhs) const {
		return level < rhs.level;
	}
	
	bool operator==(const ConsoleLogDefine& rhs) const {
		return level == rhs.level
			&& format == rhs.format;
	}
};

template <>
class LexicalCast<std::string, ConsoleLogDefine> {
public:
	ConsoleLogDefine operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap()
			|| !node["level"].IsScalar()
			|| !node["format"].IsScalar()) {
			throw LexicalCast_error("std::string to ConsoleLogDefine fmt error");
		}
		ConsoleLogDefine condf;
		condf.level = LogLevel::StringToLevel(node["level"].Scalar());
		condf.format = node["format"].Scalar();
		return condf;
	}
};

template <>
class LexicalCast<ConsoleLogDefine, std::string> {
public:
	std::string operator()(const ConsoleLogDefine& condf) const {
		YAML::Node node;
		node["level"] = LogLevel::LevelToString(condf.level);
		node["format"] = condf.format;
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template<>
class stl_util::Hash<ConsoleLogDefine> {
public:
	size_t operator()(const ConsoleLogDefine& condf) const {
		return std::hash<std::string>()(condf.format);
	}
};

//SoleLog
struct SoleLogDefine {
	LogLevel::Level level;
	std::string format;
	std::string path;

	bool operator<(const SoleLogDefine& rhs) const {
		return level < rhs.level;
	}
	
	bool operator==(const SoleLogDefine& rhs) const {
		return level == rhs.level
			&& format == rhs.format
			&& path == rhs.path;
	}
};

template <>
class LexicalCast<std::string, SoleLogDefine> {
public:
	SoleLogDefine operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap()
			|| !node["level"].IsScalar()
			|| !node["format"].IsScalar()
			|| !node["path"].IsScalar()) {
			throw LexicalCast_error("std::string to SoleLogDefine fmt error");
		}
		SoleLogDefine soledf;
		soledf.level = LogLevel::StringToLevel(node["level"].Scalar());
		soledf.format = node["format"].Scalar();
		soledf.path = node["path"].Scalar();
		return soledf;
	}
};

template <>
class LexicalCast<SoleLogDefine, std::string> {
public:
	std::string operator()(const SoleLogDefine& soledf) const {
		YAML::Node node;
		node["level"] = LogLevel::LevelToString(soledf.level);
		node["format"] = soledf.format;
		node["path"] = soledf.path;
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template<>
class stl_util::Hash<SoleLogDefine> {
public:
	size_t operator()(const SoleLogDefine& soledf) const {
		return std::hash<std::string>()(soledf.format);
	}
};


//DateLog
struct DateLogDefine {
	LogLevel::Level level;
	std::string format;
	std::string path;
	uint32_t maxCount;
	
	bool operator<(const DateLogDefine& rhs) const {
		return level < rhs.level;
	}
	
	bool operator==(const DateLogDefine& rhs) const {
		return level == rhs.level
			&& format == rhs.format
			&& path == rhs.path
			&& maxCount == rhs.maxCount;
	}
};

template <>
class LexicalCast<std::string, DateLogDefine> {
public:
	DateLogDefine operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap()
			|| !node["level"].IsScalar()
			|| !node["format"].IsScalar()
			|| !node["path"].IsScalar()
			|| !node["maxCount"].IsScalar()) {
			throw LexicalCast_error("std::string to DateLogDefine fmt error");
		}
		DateLogDefine datedf;
		datedf.level = LogLevel::StringToLevel(node["level"].Scalar());
		datedf.format = node["format"].Scalar();
		datedf.path = node["path"].Scalar();
		datedf.maxCount = LexicalCast<std::string, uint32_t>()(node["maxCount"].Scalar());
		return datedf;
	}
};

template <>
class LexicalCast<DateLogDefine, std::string> {
public:
	std::string operator()(const DateLogDefine& datedf) const {
		YAML::Node node;
		node["level"] = LogLevel::LevelToString(datedf.level);
		node["format"] = datedf.format;
		node["path"] = datedf.path;
		node["maxCount"] = LexicalCast<uint32_t, std::string>()(datedf.maxCount);
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template<>
class stl_util::Hash<DateLogDefine> {
public:
	size_t operator()(const DateLogDefine& datedf) const {
		return std::hash<std::string>()(datedf.format);
	}
};


//SizeLog
struct SizeLogDefine {
	LogLevel::Level level;
	std::string format;
	std::string path;
	uint32_t maxCount;
	std::string maxSize;
	
	bool operator<(const SizeLogDefine& rhs) const {
		return level < rhs.level;
	}
	
	bool operator==(const SizeLogDefine& rhs) const {
		return level == rhs.level
			&& format == rhs.format
			&& path == rhs.path
			&& maxCount == rhs.maxCount
			&& maxSize == rhs.maxSize;
	}
};

template <>
class LexicalCast<std::string, SizeLogDefine> {
public:
	SizeLogDefine operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap()
			|| !node["level"].IsScalar()
			|| !node["format"].IsScalar()
			|| !node["path"].IsScalar()
			|| !node["maxCount"].IsScalar()
			|| !node["maxSize"].IsScalar()) {
			throw LexicalCast_error("std::string to SizeLogDefine fmt error");
		}
		SizeLogDefine sizedf;
		sizedf.level = LogLevel::StringToLevel(node["level"].Scalar());
		sizedf.format = node["format"].Scalar();
		sizedf.path = node["path"].Scalar();
		sizedf.maxCount = LexicalCast<std::string, uint32_t>()(node["maxCount"].Scalar());
		sizedf.maxSize = node["maxSize"].Scalar();

		return sizedf;
	}
};

template <>
class LexicalCast<SizeLogDefine, std::string> {
public:
	std::string operator()(const SizeLogDefine& sizedf) const {
		YAML::Node node;
		node["level"] = LogLevel::LevelToString(sizedf.level);
		node["format"] = sizedf.format;
		node["path"] = sizedf.path;
		node["maxCount"] = LexicalCast<uint32_t, std::string>()(sizedf.maxCount);
		node["maxSize"] = sizedf.maxSize;
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template<>
class stl_util::Hash<SizeLogDefine> {
public:
	size_t operator()(const SizeLogDefine& sizedf) const {
		return std::hash<std::string>()(sizedf.format);
	}
};


//Logger
struct LoggerDefine {
	std::string owner;
	LogLevel::Level level;
	std::string format;
	List<ConsoleLogDefine> condfs;
	List<SoleLogDefine> soledfs;
	List<DateLogDefine> datedfs;
	List<SizeLogDefine> sizedfs;
	
	bool operator<(const LoggerDefine& rhs) const {
		return owner < rhs.owner;
	}
	
	bool operator==(const LoggerDefine& rhs) const {
		return owner == rhs.owner
			&& level == rhs.level
			&& format == rhs.format
			&& condfs == rhs.condfs
			&& soledfs == rhs.soledfs
			&& datedfs == rhs.datedfs
			&& sizedfs == rhs.sizedfs;
	}
};

template <>
class LexicalCast<std::string, LoggerDefine> {
public:
	LoggerDefine operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap()
			|| !node["owner"].IsScalar()
			|| !node["level"].IsScalar()
			|| !node["format"].IsScalar()
			|| (node["ConsoleLogAppender"].IsDefined() ? !(node["ConsoleLogAppender"].IsSequence() || node["ConsoleLogAppender"].IsNull()) : false)
			|| (node["SoleFileLogAppender"].IsDefined() ? !(node["SoleFileAppender"].IsSequence() || node["SoleFileAppender"].IsNull()) : false)
			|| (node["DateRotateFileAppender"].IsDefined() ? !(node["DateRotateFileAppender"].IsSequence() || node["DateRotateFileAppender"].IsNull()) : false)
			|| (node["SizeRotateFileAppender"].IsDefined() ? !(node["SizeRotateFileAppender"].IsSequence() || node["SizeRotateFileAppender"].IsNull()) : false)) {
			throw LexicalCast_error("std::string to LoggerDefine fmt error");
		}
		LoggerDefine loggerdf;
		loggerdf.owner = node["owner"].Scalar();
		loggerdf.level = LogLevel::StringToLevel(node["level"].Scalar());
		loggerdf.format = node["format"].Scalar();
		for(size_t i = 0; i < node["ConsoleLogAppender"].size(); ++i) {
			std::ostringstream oss;
			oss << node["ConsoleLogAppender"][i];
			ConsoleLogDefine condf = LexicalCast<std::string, ConsoleLogDefine>()(oss.str());
			loggerdf.condfs.push_back(condf);
		}
		
		for(size_t i = 0; i < node["SoleFileLogAppender"].size(); ++i) {
			std::ostringstream oss;
			oss << node["SoleFileLogAppender"][i];
			SoleLogDefine soledf = LexicalCast<std::string, SoleLogDefine>()(oss.str());
			loggerdf.soledfs.push_back(soledf);
		}

		for(size_t i = 0; i < node["DateRotateFileAppender"].size(); ++i) {
			std::ostringstream oss;
			oss << node["DateRotateFileAppender"][i];
			DateLogDefine datedf = LexicalCast<std::string, DateLogDefine>()(oss.str());
			loggerdf.datedfs.push_back(datedf);
		}

		for(size_t i = 0; i < node["SizeRotateFileAppender"].size(); ++i) {
			std::ostringstream oss;
			oss << node["SizeRotateFileAppender"][i];
			SizeLogDefine sizedf = LexicalCast<std::string, SizeLogDefine>()(oss.str());
			loggerdf.sizedfs.push_back(sizedf);
		}
		
		return loggerdf;
	}
};

template <>
class LexicalCast<LoggerDefine, std::string> {
public:
	std::string operator()(const LoggerDefine& loggerdf) const {
		YAML::Node node;
		node["owner"] = loggerdf.owner;
		node["level"] = LogLevel::LevelToString(loggerdf.level);
		node["format"] = loggerdf.format;
		node["ConsoleLogAppender"] = YAML::Load(LexicalCast<List<ConsoleLogDefine>, std::string>()(loggerdf.condfs));
        node["SoleFileLogAppender"] = YAML::Load(LexicalCast<List<SoleLogDefine>, std::string>()(loggerdf.soledfs));
        node["DateRotateFileAppender"] = YAML::Load(LexicalCast<List<DateLogDefine>, std::string>()(loggerdf.datedfs));
        node["SizeRotateFileAppender"] = YAML::Load(LexicalCast<List<SizeLogDefine>, std::string>()(loggerdf.sizedfs));
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template<>
class stl_util::Hash<LoggerDefine> {
public:
	size_t operator()(const LoggerDefine& loggerdf) const {
		return std::hash<std::string>()(loggerdf.owner);
	}
};


static void restore_root() {
	Logger::ptr root = WEBSERVER_LOGMGR_ROOT();
	root->setLevel(LogLevel::DEBUG);
	LogFormatter::ptr formatter(new LogFormatter());
	root->setFormatter(formatter);
	root->clearAppenders();
	ConsoleAppender::ptr console(new ConsoleAppender());
	root->addAppender(console);
}

static void set_logger(const LoggerDefine& loggerdf, Logger::ptr logger) {
	logger->setLevel(loggerdf.level);
	LogFormatter::ptr formatter(new LogFormatter(loggerdf.format));
	logger->setFormatter(formatter);
	logger->clearAppenders();
	
	for(auto& it : loggerdf.condfs) {
		ConsoleAppender::ptr console(new ConsoleAppender());
		console->setLevel(it.level);
		if(it.format != "default") {
			LogFormatter::ptr formatter(new LogFormatter(it.format));
			console->setFormatter(formatter);
		}
		logger->addAppender(console);
	}
	for(auto& it : loggerdf.soledfs) {
		SoleFileAppender::ptr sole(new SoleFileAppender(it.path));
		sole->setLevel(it.level);
		if(it.format != "default") {
			LogFormatter::ptr formatter(new LogFormatter(it.format));
			sole->setFormatter(formatter);
		}
		logger->addAppender(sole);
	}
	for(auto& it : loggerdf.datedfs) {
		DateRotateFileAppender::ptr date(new DateRotateFileAppender(it.path, it.maxCount));
		date->setLevel(it.level);
		if(it.format != "default") {
			LogFormatter::ptr formatter(new LogFormatter(it.format));
			date->setFormatter(formatter);
		}
		logger->addAppender(date);
	}
	for(auto& it : loggerdf.sizedfs) {
		SizeRotateFileAppender::ptr size(new SizeRotateFileAppender(it.path, it.maxSize, it.maxCount));
		size->setLevel(it.level);
		if(it.format != "default") {
			LogFormatter::ptr formatter(new LogFormatter(it.format));
			size->setFormatter(formatter);
		}
		logger->addAppender(size);
	}
}


//init
struct LogConfig_Initer {
	using Type = Unordered_set<LoggerDefine>; 

	LogConfig_Initer() {
		auto var = ConfigSingleton::GetInstance()
			->addConfigVar<Type>("logs", Type{}, "this configvar defined logs configs");

		var->addMonitor("log_01", [](const Type& oldVar, const Type& newVar) {
			for(auto& it : oldVar) {
				auto pending_logger = std::find_if(newVar.begin(), newVar.end(), 
						[=](const LoggerDefine& rhs)->bool {
					return it.owner == rhs.owner;
				});

				if(pending_logger == newVar.end()) {	//del
					if(it.owner == "root") {
						restore_root();
						continue;
					}
					WEBSERVER_LOGMGR_DEL(it.owner);
				} else {								//change
					if(it.owner == "root") {
						Logger::ptr root = WEBSERVER_LOGMGR_ROOT();
						set_logger(it, root);
						continue;
					}

					Logger::ptr ch_logger = WEBSERVER_LOGMGR_GET(it.owner);
					set_logger(it, ch_logger);
				}
			}
			
			for(auto& it : newVar) {
				auto pending_logger = std::find_if(oldVar.begin(), oldVar.end(), 
						[=](const LoggerDefine& rhs)->bool {
					return it.owner == rhs.owner;
				});
				
				if(pending_logger == oldVar.end()) {	//add
					if(it.owner == "root") {
						Logger::ptr root = WEBSERVER_LOGMGR_ROOT();
						set_logger(it, root);
						continue;
					}				

					Logger::ptr new_logger(new Logger(it.owner));
					set_logger(it, new_logger);
					WEBSERVER_LOGMGR_ADD(new_logger);
				}
			}
		});
	}
};

static LogConfig_Initer LogConfig_Initer_CXX_01;

}	// namespace webserver
