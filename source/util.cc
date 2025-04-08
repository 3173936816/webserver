#include "util.h"
#include "macro.h"
#include "thread.h"
#include "coroutine.h"
#include "time.h"

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <time.h>
#include <execinfo.h>
#include <sys/time.h>
#include <stdint.h>
#include <random>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace webserver {

int DIR_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::DIRECTORY;
}

int CHAR_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::CHAR;
}

int BLOCK_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::BLOCK;
}

int REGULAR_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::REGULAR;
}

int FIFO_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::FIFO;
}

int LINK_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::LINK;
}

int SOCK_filter(const struct dirent* direnty) {
	return direnty->d_type == FileType::SOCK;
}

int Alpha_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return alphasort(lhs, rhs);
}

int Version_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return versionsort(lhs, rhs); 
}

static int date_compar(const struct dirent** lhs, const struct dirent** rhs) {
	std::string lhs_name = (*lhs)->d_name;
	std::string rhs_name = (*rhs)->d_name;
	
	size_t lhs_point_pos = lhs_name.rfind('.');
	size_t lhs_slash_pos = lhs_name.rfind('/');
	size_t rhs_point_pos = rhs_name.rfind('.');
	size_t rhs_slash_pos = rhs_name.rfind('/');
	
	std::string lhs_date = lhs_name.substr(lhs_slash_pos + 1, lhs_point_pos);
	std::string rhs_date = rhs_name.substr(rhs_slash_pos + 1, rhs_point_pos);
	
	return ::strcmp(lhs_date.c_str(), rhs_date.c_str());	
}

int Date_lower_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return date_compar(lhs, rhs);
}

int Date_upper_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return 0 - date_compar(lhs, rhs);
}

//static std::string directory{};

static int Size_compar(const struct dirent** lhs, const struct dirent** rhs) {	
	std::string lhs_name = (*lhs)->d_name;
	std::string rhs_name = (*rhs)->d_name;
	
	size_t pos1 = lhs_name.rfind("_");
	size_t pos2 = lhs_name.rfind(".");
	uint32_t lhs_num = std::atoi(lhs_name.substr(pos1 + 1, pos2 - pos1 - 1).c_str());

	pos1 = rhs_name.rfind("_");
	pos2 = rhs_name.rfind(".");
	uint32_t rhs_num = std::atoi(rhs_name.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
	
	if(lhs_num > rhs_num) return 1;
	else if(lhs_num < rhs_num) return -1;
	else return 0;
}

int Size_lower_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return Size_compar(lhs, rhs);
}

int Size_upper_compar(const struct dirent** lhs, const struct dirent** rhs) {
	return 0 - Size_compar(lhs, rhs);
}

void FileSelect(const std::string& directory, 
		DirentList& direntList, Filter filter, Compar compar) {
//	webserver::directory = directory;
	struct dirent** dirents = nullptr;
	int rt = scandir(directory.c_str(), &dirents, filter, compar);
	if(WEBSERVER_UNLIKELY(rt == -1)) {
		LIBFUN_ABORT_LOG(rt, "FileSelect error");
	}
	for(int32_t i = 0; i < rt; ++i) {
		direntList.emplace_back(dirents[i], [](struct dirent* ptr){
			free(ptr);
		});
	}
	free(dirents);
}

void LogFileSelect(const std::string& directory, const std::string& suffix, 
		DirentList& direntList, Compar compar) {
	DirentList candidateList;
	FileSelect(directory, candidateList, REGULAR_filter, compar);
	for(size_t i = 0; i < candidateList.size(); ++i) {
		std::string name = candidateList[i]->d_name;
		std::string name_suffix = name.substr(name.rfind('.'));
		if(name_suffix == suffix) {
			direntList.push_back(std::move(candidateList[i]));
		}
	}
}

std::string GetDate() {
	struct tm* tmp = nullptr;
	time_t t = time(nullptr);
	
	tmp = localtime(&t);
	if(WEBSERVER_UNLIKELY(!tmp)) {
		LIBFUN_ABORT_LOG(errno, "GetDate--localtime error");
	}

	char buff[48] = {0};
	int32_t rt = strftime(buff, sizeof(buff), "%Y-%m-%d", tmp);
	//int32_t rt = strftime(buff, sizeof(buff), "%Y-%m-%S", tmp);
	if(WEBSERVER_UNLIKELY(!rt)) {
		LIBFUN_ABORT_LOG(rt, "GetDate--localtime error");
	}

	return std::string(buff, rt);
}

uint64_t StringCalculate(const std::string& formula) {
	std::istringstream iss(formula);
	uint64_t lhs, rhs;
	std::string op;
	iss >> lhs;
	
	while(iss >> op >> rhs) {
		switch(op[0]){
			case '+': {
				lhs += rhs;
				break;
			}
			case '-': {
				lhs -= rhs;
				break;
			}
			case '*': {
				lhs *= rhs;
				break;
			}
			case '/': {
				lhs /= rhs;
				break;
			}
			default: {
				return 0;
			}
		}
	}
	return lhs;
}

off_t GetFileSize(const std::string& path) {
	struct stat st;
	int32_t rt = ::stat(path.c_str(), &st);
	if(WEBSERVER_UNLIKELY(rt)) {
		throw std::invalid_argument(path.c_str());
	}
	return st.st_size;
}

uint32_t GetThreadTid() {
	return syscall(SYS_gettid);
}

std::string GetThreadName() {
	return now_thread::GetThreadName();
}

uint32_t GetCoroutineId() {
	return now_coroutine::GetCid();
}

uint32_t GetUUID() {
//	static uint32_t i = 0;
//	i %= ~0x00;
//	return ++i;

	static boost::uuids::random_generator gen;	

	boost::uuids::uuid uuid = gen();
    uint32_t num = *reinterpret_cast<const uint32_t*>(uuid.data);
	
	 uint32_t maxVal = 1;
     for(int i = 0; i < 5; ++i) {
     	maxVal *= 10;
     }
     num %= maxVal;	

    return num;
}

static void BacktraceSymbols(void*(&buff)[1024], std::vector<std::string>& vec, uint32_t layerNum) {
	char** rt = backtrace_symbols(buff, 1024);
	if(WEBSERVER_UNLIKELY(rt == nullptr)) {
		LIBFUN_STD_LOG(errno, "BacktraceSymbols -- backstrace_symbols error");
		return;
	}

	char** tmp = rt;
	for(size_t i = 0; i < layerNum && tmp != nullptr; ++i, ++tmp) {
		vec.emplace_back(*tmp);
	}
	free(rt);
}


std::string PrintStackInfo(uint32_t layerNum) {
	void* buff[1024] = {0};
	int32_t rt = backtrace(buff, 1024);
	if(WEBSERVER_UNLIKELY(!rt)) {
		LIBFUN_ABORT_LOG(rt, "PrintSatckInfo -- backstrace error");
	}
	std::vector<std::string> vec;
	BacktraceSymbols(buff, vec, layerNum);
	
	std::ostringstream oss;
	for(auto& it : vec) {
		oss << "-    " << it << '\n';
	}
	return oss.str();
}

uint64_t GetMs() {
	Timeval tv;
	int32_t rt = gettimeofday(tv.get(), nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "GetMs--gettimeofday error");
	}
	return tv.seconds() * 1000 + tv.microseconds() / 1000;
}

uint64_t GetUs() {
	Timeval tv;
	int32_t rt = gettimeofday(tv.get(), nullptr);
	if(WEBSERVER_UNLIKELY(rt)) {
		LIBFUN_ABORT_LOG(rt, "GetUs--gettimeofday error");
	}
	return tv.seconds() * 1000 * 1000 + tv.microseconds();
}

}	// namespace webserver
