#include "endian_convert.h"

namespace webserver {

uint16_t ByteSwap16(uint16_t value) {
	return bswap_16(value);
}

uint32_t ByteSwap32(uint32_t value) {
	return bswap_32(value);
}

uint64_t ByteSwap64(uint64_t value) {
	return bswap_64(value);
}

uint16_t NetworkToHost16(uint16_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap16(value);
#endif	
}

uint32_t NetworkToHost32(uint32_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap32(value);
#endif	
}

uint64_t NetworkToHost64(uint64_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap64(value);
#endif	
}

uint16_t HostToNetwork16(uint16_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap16(value);
#endif	
}

uint32_t HostToNetwork32(uint32_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap32(value);
#endif	
}

uint64_t HostToNetwork64(uint64_t value) {
#if WEBSERVER_BYTE_ORDER == WEBSERVER_BIG_ENDIAN
	return value;
#else
	return ByteSwap64(value);
#endif	
}

}	// namespace webserver
