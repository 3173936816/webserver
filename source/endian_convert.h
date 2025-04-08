#ifndef __WEBSERVER_ENDIAN_CONVERT_H__
#define __WEBSERVER_ENDIAN_CONVERT_H__

#include <stdint.h>
#include <byteswap.h>
#include <endian.h>

#define WEBSERVER_BYTE_ORDER 		__BYTE_ORDER
#define WEBAERVER_BIG_ENDIAN 		__BIG_ENDIAN
#define WEBAERVER_LITTILE_ENDIAN	__LITTLE_ENDIAN

namespace webserver {

uint16_t ByteSwap16(uint16_t value);
uint32_t ByteSwap32(uint32_t value);
uint64_t ByteSwap64(uint64_t value);

uint16_t NetworkToHost16(uint16_t value);
uint32_t NetworkToHost32(uint32_t value);
uint64_t NetworkToHost64(uint64_t value);

uint16_t HostToNetwork16(uint16_t value);
uint32_t HostToNetwork32(uint32_t value);
uint64_t HostToNetwork64(uint64_t value);

}	// namespace webserver

#endif // __WEBSERVER_ENDIAN_CONVERT_H__
