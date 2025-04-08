#include <iostream>
#include "webserver.h"

using namespace webserver;

void test_level() {
	std::cout << LogLevel::LevelToString(static_cast<LogLevel::Level>(8)) << std::endl;
	std::cout << LogLevel::StringToLevel("FATALL") << std::endl;
}

void test_console() {	
	LogEvent::ptr event(new LogEvent(time(0), __FILE__, __LINE__, 0, "ppp", 0));
	event->getStream() << "log test";
	Logger::ptr logger(new Logger("root"));
	ConsoleAppender::ptr console(new ConsoleAppender());	
	console->setLevel(LogLevel::ERROR);

	LogFormatter::ptr formatter(new LogFormatter("%D%T%T%f:%l%T%m%n"));
	console->setFormatter(formatter);

	logger->addAppender(console);

	logger->log(LogLevel::FATAL, event);
}

void test_sole() {
	LogEvent::ptr event(new LogEvent(time(0), __FILE__, __LINE__, 0, "ppp", 0));
	event->getStream() << "log test";

	SoleFileAppender::ptr sole(new SoleFileAppender("../logs/test.txt"));

	Logger::ptr logger(new Logger("root"));
	logger->addAppender(sole);

	logger->log(LogLevel::FATAL, event);
}

void test_selectFile() {
	DirentList direntList;
	LogFileSelect("../logs/", ".log", direntList, Size_lower_compar);
	for(size_t i = 0; i < direntList.size(); ++i) {
		std::cout << direntList[i]->d_name << std::endl;
	}
}

void test_GetDate() {
	std::cout << GetDate() << std::endl;
}

void test_DateLog() {
	LogEvent::ptr event(new LogEvent(time(0), __FILE__, __LINE__, 0, "ppp", 0));
	event->getStream() << "log test";

	DateRotateFileAppender::ptr date(
		new DateRotateFileAppender("../logs/datelog/test", 5));
	
	Logger::ptr logger(new Logger("root"));
	logger->addAppender(date);
	
	while(1) {
		logger->log(LogLevel::FATAL, event);
	}
}

long long i = 0;
void test_SizeLog() {
	SizeRotateFileAppender::ptr size(
		new SizeRotateFileAppender("../logs/sizelog/test", "1024 * 1024", 5));
	
	Logger::ptr logger(new Logger("root"));
	logger->addAppender(size);
	
	while(i != 80) {
		LogEvent::ptr event(new LogEvent(time(0), __FILE__, __LINE__, 0, "ppp", 0));
		event->getStream() << "log test " << ++i;
		logger->log(LogLevel::FATAL, event);
	}
}

static webserver::Logger::ptr g_logger = WEBSERVER_LOGMGR_GET("system");

void test_logWarp() {
	Logger::ptr logger(new Logger("system"));
	ConsoleAppender::ptr console(new ConsoleAppender());
	logger->addAppender(console);
	
	WEBSERVER_LOGMGR_ADD(logger);

	WEBSERVER_LOG_LEVEL(webserver::LogLevel::DEBUG, logger) << "webserver_log_level";
	WEBSERVER_LOG_FATAL(logger) << "webserver_log_level fatal";
	WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET("system")) << "webserver_log_level fatal";
	
	WEBSERVER_LOGMGR_DEL("system");

	WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET("system")) << "webserver_log_level LLLL";
	WEBSERVER_FMT_FATAL(WEBSERVER_LOGMGR_ROOT(), "hhh %d jjj %s", 1000, "ppppp");
}

int main() {
	//test_level();
	//test_console();
	//test_sole();
	//test_selectFile();
	//test_GetDate();
	//test_DateLog();
	//test_SizeLog();
	test_logWarp();
	
	return 0;
}
