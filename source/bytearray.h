#ifndef __WEBSERVER_BYTEARRAY_H__
#define __WEBSERVER_BYTEARRAY_H__

#include <memory>
#include <list>
#include <vector>
#include <sys/uio.h>

namespace webserver {

class ByteArray {
public:
	using ptr = std::shared_ptr<ByteArray>;
	
	explicit ByteArray(uint64_t baseSize = 512 * 512);
	~ByteArray();
	
	//	write relatd
	void writeFixUint8(uint8_t v);
	void writeFixUint16(uint16_t v);
	void writeFixUint32(uint32_t v);
	void writeFixUint64(uint64_t v);
	
	void writeFixInt8(int8_t v);
	void writeFixInt16(int16_t v);
	void writeFixInt32(int32_t v);
	void writeFixInt64(int64_t v);
	
	void writeVarUint8(uint8_t v);
	void writeVarUint16(uint16_t v);
	void writeVarUint32(uint32_t v);
	void writeVarUint64(uint64_t v);
	
	void writeVarInt8(int8_t v);
	void writeVarInt16(int16_t v);
	void writeVarInt32(int32_t v);
	void writeVarInt64(int64_t v);
	
	void writeFloat(float v);
	void writeDouble(double v);
	
	void writeStringAsC(const std::string& str);
	void writeString(const std::string& str, uint64_t len);
	void writeStringWithLength(const std::string& str);
	
	//	read related
	uint8_t readFixUint8();
	uint16_t readFixUint16();
	uint32_t readFixUint32();
	uint64_t readFixUint64();
	
	int8_t readFixInt8();
	int16_t readFixInt16();
	int32_t readFixInt32();
	int64_t readFixInt64();
	
	uint8_t readVarUint8();
	uint16_t readVarUint16();
	uint32_t readVarUint32();
	uint64_t readVarUint64();
	
	int8_t readVarInt8();
	int16_t readVarInt16();
	int32_t readVarInt32();
	int64_t readVarInt64();
	
	float readFloat();
	double readDouble();
	
	std::string readStringAsC();
	std::string readString(uint64_t len);
	std::string readStringWithLength();
	
	// peek related
	uint8_t peekFixUint8() const;
	uint16_t peekFixUint16() const;
	uint32_t peekFixUint32() const;
	uint64_t peekFixUint64() const;
	
	int8_t peekFixInt8() const;
	int16_t peekFixInt16() const;
	int32_t peekFixInt32() const;
	int64_t peekFixInt64() const;
	
	uint8_t peekVarUint8() const;
	uint16_t peekVarUint16() const;
	uint32_t peekVarUint32() const;
	uint64_t peekVarUint64() const;
	
	int8_t peekVarInt8() const;
	int16_t peekVarInt16() const;
	int32_t peekVarInt32() const;
	int64_t peekVarInt64() const;
	
	float peekFloat() const;
	double peekDouble() const;
	
	std::string peekStringAsC() const;
	std::string peekString(uint64_t len) const;
	std::string peekStringWithLength() const;
	
	// other related
	void addCapacity(uint32_t mul);
	void shrink_to_fit();
	void clear();
		
	uint64_t getBaseSize() const;
	uint64_t getDataSize() const;
	uint64_t getCapacity() const;
	
	bool writeToFile(const std::string& path);
	bool readFromFile(const std::string& path);
	bool peekToFile(const std::string& path) const;
	
	bool writeToFileHex(const std::string& path);
	bool readFromFileHex(const std::string& path);
	bool peekToFileHex(const std::string& path) const;
	
	bool getReadBuffers(std::vector<struct iovec>& iovecs, uint64_t len);
	bool getWriteBuffers(std::vector<struct iovec>& iovecs, uint64_t len);
	bool restorative(uint64_t byteSize);

private:
	void writeToBlock(const void* buff, uint64_t len);
	void readFromBlock(void* buff, uint64_t len);
	void peekFromBlock(void* buff, uint64_t len, uint64_t offset) const;

private:
	struct DataBlock {
		uint8_t* data;
		
		DataBlock(uint64_t baseSize);
		DataBlock(DataBlock&& dataBlock);
		~DataBlock();
	};

private:
	using Iterator = std::list<DataBlock>::iterator;

	bool m_isInit;
	bool m_fail;
	uint64_t m_baseSize;
	uint64_t m_dataSize;
	uint64_t m_capacity;
	uint64_t m_readIndex;
	uint64_t m_writeIndex;
	Iterator m_readIterator;
	Iterator m_writeIterator;
	std::list<DataBlock> m_dataBlocks;
};

}	// namespace webserver

#endif // ! __WEBSERVER_BYTEARRAY_H__
