#include "webserver.h"

using namespace webserver;

void test_alloc() {
	//uint64_t v = 11327;
	//ByteArray::ptr by = std::make_shared<ByteArray>(1);
	//by->writeToBlock(&v, 8);
	//
	//uint64_t m = 0;
	//by->copyBlocks(&m, 8);
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << (int)m;
	//
	//uint64_t k = 0;
	//by->readFromBlock(&k, 8);
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << (int)k;

	//
	//by->addCapacity(10);
	//by->clear();
	//by->shrink_to_fit();
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << by->getCapacity();
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << by->getDataSize();
	//
	//by->writeToBlock(&v, 8);
	//
	//m = 0;
	//by->copyBlocks(&m, 8);
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << (int)m;
	//
	//k = 0;
	//by->readFromBlock(&k, 8);
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << (int)k;
}

void test_convert() {
	ByteArray::ptr by = std::make_shared<ByteArray>(5);
	
#define XX(name, val)	\
	{	\
		by->name(val);	\
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())	\
			 << #name << ": " << val;	\
	}

#define XX2(name)	\
	{	\
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())	\
			 << #name << ": " << by->name();	\
	}

#define XX3(name)	\
	{	\
		WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())	\
			 << #name << ": " << by->name();	\
	}
	
	
	XX(writeFixUint8, 88)
	XX3(peekFixUint8);
	XX2(readFixUint8)
	XX(writeFixUint16, 267)
	XX3(peekFixUint16);
	XX2(readFixUint16)
	XX(writeFixUint32, 2258)
	XX3(peekFixUint32);
	XX2(readFixUint32)
	XX(writeFixUint64, 9925)
	XX3(peekFixUint64);
	XX2(readFixUint64)
	
	XX(writeFixInt8, -6)
	XX3(peekFixInt8)
	XX2(readFixInt8)
	XX(writeFixInt16, -96)
	XX3(peekFixInt16)
	XX2(readFixInt16)
	XX(writeFixInt32, -256)
	XX3(peekFixInt32)
	XX2(readFixInt32)
	XX(writeFixInt64, -888)
	XX3(peekFixInt64)
	XX2(readFixInt64)
	
	XX(writeVarUint8, 65)
	XX3(peekVarUint8)
	XX2(readVarUint8)
	XX(writeVarUint16, 666)
	XX3(peekVarUint16)
	XX2(readVarUint16)
	XX(writeVarUint32, 36)
	XX3(peekVarUint32)
	XX2(readVarUint32)
	XX(writeVarUint64, 555)
	XX3(peekVarUint64)
	XX2(readVarUint64)
	
	XX(writeVarInt8, -8)
	XX3(peekVarInt8)
	XX2(readVarInt8)
	XX(writeVarInt16, -9)
	XX3(peekVarInt16)
	XX2(readVarInt16)
	XX(writeVarInt32, -777)
	XX3(peekVarInt32)
	XX2(readVarInt32)
	XX(writeVarInt64, -9999)
	XX3(peekVarInt64)
	XX2(readVarInt64)
	
	XX(writeFloat, 1.23)
	XX3(peekFloat)
	XX2(readFloat)
	XX(writeDouble, 258.99)
	XX3(peekDouble)
	XX2(readDouble)
	
	std::cout << std::endl;	
	
	XX(writeStringAsC, "hhhhh")
	XX3(peekStringAsC)
	XX2(readStringAsC)
	
	XX(writeStringWithLength, "hello world")
	XX3(peekStringWithLength)
	XX2(readStringWithLength)
}

void test_file() {
	ByteArray::ptr by = std::make_shared<ByteArray>(5);
	by->addCapacity(100);

	XX(writeFixUint8, 88)
	XX(writeFixUint16, 267)
	XX(writeFixUint32, 2258)
	XX(writeFixUint64, 9925)
	XX(writeFixInt8, -6)
	XX(writeFixInt16, -96)
	XX(writeFixInt32, -256)
	XX(writeFixInt64, -888)
	XX(writeVarUint8, 65)
	XX(writeVarUint16, 666)
	XX(writeVarUint32, 36)
	XX(writeVarUint64, 555)
	XX(writeVarInt8, -8)
	XX(writeVarInt16, -9)
	XX(writeVarInt32, -777)
	XX(writeVarInt64, -9999)
	XX(writeFloat, 1.23)
	XX(writeDouble, 258.99)
	XX(writeStringAsC, "hhhhh")
	XX(writeStringWithLength, "hello world")

	//by->peekToFile("../tests/dat/bytearray.dat");
	//by->writeToFileHex("../tests/dat/bytearray.dat");
	//by->readFromFileHex("../tests/dat/bytearray.dat");
	//by->peekToFileHex("../tests/dat/bytearray.dat");
	
	XX2(readFixUint8)
	XX2(readFixUint16)
	XX2(readFixUint32)
	XX2(readFixUint64)
	XX2(readFixInt8)
	XX2(readFixInt16)
	XX2(readFixInt32)
	XX2(readFixInt64)
	XX2(readVarUint8)
	XX2(readVarUint16)
	XX2(readVarUint32)
	XX2(readVarUint64)
	XX2(readVarInt8)
	XX2(readVarInt16)
	XX2(readVarInt32)
	XX2(readVarInt64)
	XX2(readFloat)
	XX2(readDouble)
	XX2(readStringAsC)
	XX2(readStringWithLength)
	
	std::cout << "capacity : "<< by->getCapacity() << std::endl;
}

void test_iovec() {
	ByteArray::ptr by = std::make_shared<ByteArray>(4);
	by->writeFixUint32(999);

	std::vector<struct iovec> vec;
	int rt = by->getWriteBuffers(vec, 10);
	if(!rt) {
		std::cout << "getWriteBuffers error" << std::endl;
		return;
	}
	uint64_t p = 9996;
	memcpy(vec[0].iov_base, &p, 4);
	memcpy(vec[1].iov_base, (((uint8_t*)&p) + 4), 4);
	rt = by->restorative(8);
	if(!rt) {
		std::cout << "restorative error" << std::endl;
		return;
	}
	std::cout << by->getDataSize() << std::endl;
	
	vec.clear();
	rt = by->getReadBuffers(vec, 10);
	if(!rt) {
		std::cout << "getReadBuffers error" << std::endl;
		return;
	}
	std::cout << by->readFixUint32() << std::endl;
	std::cout << by->readFixUint64() << std::endl;
	
	
	by->writeFixUint32(64);
	std::cout << by->readFixUint32() << std::endl;
}

void test() {
	Socket::ptr sock = std::make_shared<Socket>(Domain::IPv4, SockType::TCP);
	Address::ptr addr = IPv4Address::ResoluteAnyTCP("www.baidu.com", "80");
	
	int rt = sock->connect_with_timeout(addr, 5000);
	if(rt < 0) {
		std::cout << "connect error : " << strerror(errno) << std::endl;
		return;
	}
	
	rt = sock->send("GET / HTTP/1.1\r\n\r\n", sizeof("GET / HTTP/1.1\r\n\r\n"));	
	if(rt < 0) {
		std::cout << "send error : " << strerror(errno) << std::endl;
		return;
	}
	
	ByteArray::ptr by = std::make_shared<ByteArray>();
	std::vector<struct iovec> vec;
	by->getWriteBuffers(vec, 512 * 512 * 2);

	rt = sock->readv(&vec[0], vec.size());
	if(rt < 0) {
		std::cout << "recv error : " << strerror(errno) << std::endl;
		return;
	}
	by->restorative(rt);
	
	std::cout << "dataSize : " << by->getDataSize() << " recv_size : " << rt << std::endl;
	std::string recv_str = by->readString(rt);
	std::cout << "dataSize : " << by->getDataSize() << " recv_size : " << rt << std::endl;
	
	//WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT())	<< recv_str;
}

int main() {
	//test_alloc();
	//test_convert();
	//test_file();
	test_iovec();
	test();

	return 0;
}
