#ifndef __WERSERVER_WEBSERVER_H__
#define __WEBSERVER_WEBSERVER_H__

#if defined(_MSC_VER) ||                                            \
     (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
      (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "macro.h"
#include "synchronism.h"
#include "time.h"
#include "exception.h"
#include "noncopyable.h"
#include "log.h"
#include "util.h"
#include "config.h"
#include "thread.h"
#include "coroutine.h"
#include "iobase.h"
#include "epollio.h"
#include "async.h"
#include "address.h"
#include "socket.h"
#include "bytearray.h"
#include "http/http.h"
#include "http/http_parser.h"
#include "server.h"
#include "http/http_session.h"
#include "http/http_servlet.h"
#include "http/http_server.h"

#endif // !__WEBSEVER_WEBSEVER_H__
