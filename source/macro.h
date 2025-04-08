#ifndef __WEBSERVER_MACRO_H__
#define __WEBSERVER_MACRO_H__

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "log.h"

#if defined __GNUC__ || defined __llvm__
#	define WEBSERVER_LIKELY(x) 			__builtin_expect(!!(x), 1)
#	define WEBSERVER_UNLIKELY(x)		__builtin_expect(!!(x), 0)
#	define WEBSERVER_EXPECT(x, r)		__bulitin_expect((x), r)
#else 
#	define WEBSERVER_LIKELY(x) 			(x)		
#	define WEBSERVER_UNLIKELY(x)		(x)	
#	define WEBSERVER_EXPECT(x, r)		(x)
#endif // !__GNU__ || __llvm__


#define LIBFUN_STD_LOG(rt, message) \
	{	\
		std::clog << __FILE__ << ":" << __LINE__ << '\t'	\
			<< message << "---" 	\
			<< ::strerror(rt) << std::endl;	\
	}

#define LIBFUN_ABORT_LOG(rt, message)	\
	{	\
		LIBFUN_STD_LOG(rt, message);	\
		::abort();	\
	}

#define WEBSERVER_ASSERT(exp)	\
	{	\
		if(WEBSERVER_UNLIKELY(!(exp))) {	\
			WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET("system"))		\
				<< '\n' << PrintStackInfo(8);	\
			assert((exp));	\
		}	\
	}

#define WEBSERVER_ASSERT2(exp, meg)	\
	{	\
		if(WEBSERVER_UNLIKELY(!(exp))) {	\
			WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_GET("system"))		\
				<< '\n' << "## message : " << meg << '\n' \
				<< PrintStackInfo(8);	\
			assert((exp));	\
		}	\
	}


#endif // !__WEBSERVER_MACRO_H__
