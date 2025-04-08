#ifndef __WEBSERVER_UTIL_H__
#define __WEBSERVER_UTIL_H__

#include <vector>
#include <string>
#include <dirent.h>
#include <time.h>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>

namespace webserver {

enum FileType {

	DIRECTORY	=	DT_DIR,      	//	This is a block device.
    CHAR		=	DT_CHR,      	//	This is a character device.
    BLOCK		=	DT_BLK,      	//	This is a directory.
    REGULAR		=	DT_REG,     	//	This is a named pipe (FIFO).
    FIFO		=	DT_FIFO,      	//	This is a symbolic link.
    LINK 		=	DT_LNK,      	//	This is a regular file.
    SOCK		=	DT_SOCK     	//	This is a UNIX domain socket.
};

//file type filter
int DIR_filter(const struct dirent* direnty);
int CHAR_filter(const struct dirent* direnty);
int BLOCK_filter(const struct dirent* direnty);
int REGULAR_filter(const struct dirent* direnty);
int FIFO_filter(const struct dirent* direnty);
int LINK_filter(const struct dirent* direnty);
int SOCK_filter(const struct dirent* direnty);

using Filter = int(*)(const struct dirent* direnty);

//file sort
int Alpha_compar(const struct dirent** lhs, const struct dirent** rhs);
int Version_compar(const struct dirent** lhs, const struct dirent** rhs);

//log file sort compatator
int Date_lower_compar(const struct dirent** lhs, const struct dirent** rhs);
int Date_upper_compar(const struct dirent** lhs, const struct dirent** rhs);
int Size_lower_compar(const struct dirent** lhs, const struct dirent** rhs);
int Size_upper_compar(const struct dirent** lhs, const struct dirent** rhs);

using Compar = int(*)(const struct dirent** lhs, const struct dirent** rhs);

//select file
using DirentPtr = std::shared_ptr<struct dirent>; 
using DirentList = std::vector<DirentPtr>;
void FileSelect(const std::string& directory, 
		DirentList& direntList, Filter filter, Compar compar);
void LogFileSelect(const std::string& directory, const std::string& suffix, 
		DirentList& direntList, Compar compar);

//other
std::string GetDate();
uint64_t StringCalculate(const std::string& formula);
off_t GetFileSize(const std::string& path); 

//logger related
uint32_t GetThreadTid();
std::string GetThreadName();
uint32_t GetCoroutineId();

//GetUUID
uint32_t GetUUID();

//stackBacktrace
std::string PrintStackInfo(uint32_t layerNum);

//GetMs
uint64_t GetMs();

}	//namespace webserver

#endif	// !__WEBSERVER_UTIL_H__
