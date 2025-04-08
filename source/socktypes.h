#ifndef __WEBSERVER_SOCKTYPES_H__
#define __WEBSERVER_SOCKTYPES_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/un.h>
#include <netdb.h>

namespace webserver {

enum class Domain {
	ANY = AF_UNSPEC,
	IPv4 = AF_INET,
	IPv6 = AF_INET6,
	UNIX = AF_UNIX,
	ERROR = -1
};

enum class SockType {
	ANY = 0,
	TCP = SOCK_STREAM,
	UDP = SOCK_DGRAM,
	ERROR = -1
}; 

enum class Protocol {
	ANY = 0,
	PROTO_TCP = IPPROTO_TCP,
	PROTO_UDP = IPPROTO_UDP,
	ERROR = -1
};

}	// namespace webserver

#endif // ! __WEBSERVER_SOCKTYPES_H__
