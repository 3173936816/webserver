#include "bytearray.h"
#include "macro.h"

#include <type_traits>
#include <string.h>
#include <stdlib.h>

namespace webserver {

// ByteArray::DataBlock
ByteArray::DataBlock::DataBlock(uint64_t baseSize)
	: data(nullptr), dataSize(0) {
	data = (uint8_t*)malloc(baseSize);
}

ByteArray::DataBlock::~DataBlock() {
	if(data) {
		free(data);
	}
}

// ByteArray
ByteArray::ByteArray(uint64_t baseSize)
	: m_isInit(false), m_baseSize(baseSize)
	, m_dataSize(0), m_capacity(0)
	, m_readIndex(0), m_writeIndex(0)
	, m_readIterator{}, m_writeIterator{}
	, m_dataBlocks{} {
}
	
ByteArray::~ByteArray() = default;

void ByteArray::writeToBlock(const void* buff, uint64_t len) {
	while(m_capacity <= len + m_dataSize) {
		m_dataBlocks.push_back(DataBlock(m_baseSize));
		m_capacity += m_baseSize;
		if(!m_isInit) {
			m_readIterator = m_dataBlocks.begin();
			m_writeIterator = m_dataBlocks.begin();
			m_isInit = true;
		}
	}
	
	const uint8_t* buff_ptr = static_cast<const uint8_t*>(buff);
	
	while(len > 0) {
		uint8_t* data_block = m_writeIterator->data;
		uint64_t data_size = m_writeIterator->dataSize;
		uint64_t left_size = m_baseSize - data_size;

		if(left_size > len) {
			memcpy(data_block + m_writeIndex, buff_ptr, len);	
			m_writeIterator->dataSize += len;
			m_writeIndex += len;
			m_dataSize += len;
			break;
		} else {
			memcpy(data_block + m_writeIndex, buff_ptr, left_size);
			m_writeIterator->dataSize += left_size;
			WEBSERVER_ASSERT(m_writeIterator->dataSize == m_baseSize);
			++m_writeIterator;
			m_writeIndex = 0;
			m_dataSize += left_size;
			buff_ptr += left_size;
			len -= left_size;
		}
	}
}

void ByteArray::readFromBlock(void* buff, uint64_t len) {
	if(len > m_dataSize) {
		throw std::out_of_range("readFromBlock len is too long");
	}
	
	uint8_t* buff_ptr = static_cast<uint8_t*>(buff);
	
	while(len > 0) {
		uint8_t* data_block = m_readIterator->data;
		uint64_t data_size = m_readIterator->dataSize;
		if(data_size > len) {
			memcpy(buff_ptr, data_block + m_readIndex, len);
			m_readIterator->dataSize -= len;
			m_readIndex += len;
			m_dataSize -= len;
			m_capacity -= len;
			break;
		} else {
			memcpy(buff_ptr, data_block + m_readIndex, data_size);
			m_readIterator->dataSize -= len;
			WEBSERVER_ASSERT(m_readIterator->dataSize == 0);
			m_dataSize -= data_size;
			m_capacity -= data_size;
			++m_readIterator;
			m_readIndex = 0;
			len -= data_size;
			buff_ptr += data_size;
		}
	}
	m_dataBlocks.erase(m_dataBlocks.begin(), m_readIterator);
}

void ByteArray::copyBlocks(void* buff, uint64_t len) {
	if(len > m_dataSize) {
		throw std::out_of_range("copyBlocks len is too long");
	}
	
	uint8_t* buff_ptr = static_cast<uint8_t*>(buff);
	uint64_t readIndex = m_readIndex;
	Iterator readIterator = m_readIterator;
	
	while(len > 0) {
		uint8_t* data_block = readIterator->data;
		uint64_t data_size = readIterator->dataSize;
		if(data_size > len) {
			memcpy(buff_ptr, data_block + readIndex, len);
			break;
		} else {
			memcpy(buff_ptr, data_block + readIndex, data_size);
			++readIterator;
			readIndex = 0;
			len -= data_size;
			buff_ptr += data_size;
		}
	}
}

//	write relatd
void ByteArray::writeFixUint8(uint8_t v) {
	writeToBlock(&v, sizeof(uint8_t));
}

void ByteArray::writeFixUint16(uint16_t v) {
	writeToBlock(&v, sizeof(uint16_t));
}

void ByteArray::writeFixUint32(uint32_t v) {
	writeToBlock(&v, sizeof(uint32_t));
}

void ByteArray::writeFixUint64(uint64_t v) {
	writeToBlock(&v, sizeof(uint64_t));
}

void ByteArray::writeFixInt8(int8_t v) {
	writeToBlock(&v, sizeof(int8_t));
}

void ByteArray::writeFixInt16(int16_t v) {
	writeToBlock(&v, sizeof(int16_t));
}

void ByteArray::writeFixInt32(int32_t v) {
	writeToBlock(&v, sizeof(int32_t));
}

void ByteArray::writeFixInt64(int64_t v) {
	writeToBlock(&v, sizeof(int64_t));
}

template <typename UnsignedT, size_t N>
static uint32_t Encode_varint(UnsignedT v, uint8_t(&buff)[N]) {
	uint32_t need_size = ((sizeof(UnsignedT) * 8) % 7) 
			? sizeof(UnsignedT) * 8 / 7 + 1 : sizeof(UnsignedT) * 8 / 7;
	if(N < need_size) {
		throw std::out_of_range("buff is not enough");
	}

	uint32_t len = 0;
	while(v > 0x7f) {
		buff[len++] = (v & 0x7f) | 0x80;
		v >>= 7;
	}
	buff[len] = v;
	return len;
}

template <typename UnsignedT, size_t N>
static UnsignedT Dencode_varint(uint8_t(&buff)[N]) {
	UnsignedT v = 0;
	uint32_t offset = 0;
	for(size_t i = 0; i < N; ++i) {
		v |= (buff[i] & 0x7f) << offset;
		if(!(buff[i] & 0x80)) {
			break;
		}
		offset += 7;
	}
	return v;
}

template <typename SignedT>
static typename std::make_unsigned<SignedT>::type Encode_zigzag(SignedT v) {
	return (typename std::make_unsigned<SignedT>::type)((v << 1) ^ (v >> 31));
}

template <typename UnsignedT>
static typename std::make_signed<UnsignedT>::type Dencode_zigzag(UnsignedT v) {
	return (typename std::make_signed<UnsignedT>::type)((v >> 1) ^ -(v & 1));
}

void ByteArray::writeVarUint8(uint8_t v) {
	uint8_t buff[2] = {0};
	uint32_t len = Encode_varint(v, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarUint16(uint16_t v) {
	uint8_t buff[3] = {0};
	uint32_t len = Encode_varint(v, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarUint32(uint32_t v) {
	uint8_t buff[5] = {0};
	uint32_t len = Encode_varint(v, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarUint64(uint64_t v) {
	uint8_t buff[10] = {0};
	uint32_t len = Encode_varint(v, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarInt8(int8_t v) {
	uint8_t val = Encode_zigzag(v);
	uint8_t buff[2] = {0};
	uint32_t len = Encode_varint(val, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarInt16(int16_t v) {
	uint16_t val = Encode_zigzag(v);
	uint8_t buff[3] = {0};
	uint32_t len = Encode_varint(val, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarInt32(int32_t v) {
	uint32_t val = Encode_zigzag(v);
	uint8_t buff[5] = {0};
	uint32_t len = Encode_varint(val, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeVarInt64(int64_t v) {
	uint64_t val = Encode_zigzag(v);
	uint8_t buff[10] = {0};
	uint32_t len = Encode_varint(val, buff);
	writeToBlock(buff, len);
}

void ByteArray::writeFloat(float v) {
	writeToBlock(&v, sizeof(float));
}

void ByteArray::writeDouble(double v) {
	writeToBlock(&v, sizeof(double));
}

void ByteArray::writeString(const std::string& str) {
	writeToBlock(str.c_str(), str.size() + 1);
}

void ByteArray::writeStringWithLength(const std::string& str) {
	size_t len = str.size();
	writeToBlock(&len, sizeof(size_t));
	writeToBlock(str.c_str(), str.size());
}

//	read related
uint8_t ByteArray::readFixUint8() {
	uint8_t v = 0;
	readFromBlock(&v, sizeof(uint8_t));
	return v;
}

uint16_t ByteArray::readFixUint16() {
	uint16_t v = 0;
	readFromBlock(&v, sizeof(uint16_t));
	return v;
}

uint32_t ByteArray::readFixUint32() {
	uint32_t v = 0;
	readFromBlock(&v, sizeof(uint32_t));
	return v;
}

uint64_t ByteArray::readFixUint64() {
	uint64_t v = 0;
	readFromBlock(&v, sizeof(uint64_t));
	return v;
}

int8_t ByteArray::readFixInt8() {
	int8_t v = 0;
	readFromBlock(&v, sizeof(int8_t));
	return v;
}

int16_t ByteArray::readFixInt16() {
	int16_t v = 0;
	readFromBlock(&v, sizeof(int16_t));
	return v;
}

int32_t ByteArray::readFixInt32() {
	int32_t v = 0;
	readFromBlock(&v, sizeof(int32_t));
	return v;
}

int64_t ByteArray::readFixInt64() {
	int64_t v = 0;
	readFromBlock(&v, sizeof(int64_t));
	return v;
}

uint8_t ByteArray::readVarUint8() {
	uint8_t buff[2] = {0};
	for(size_t i = 0; i < 2; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint8_t v = Dencode_varint<uint8_t>(buff);

	return v;
}

uint16_t ByteArray::readVarUint16() {
	uint8_t buff[3] = {0};
	for(size_t i = 0; i < 3; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint16_t v = Dencode_varint<uint16_t>(buff);

	return v;
}

uint32_t ByteArray::readVarUint32() {
	uint8_t buff[5] = {0};
	for(size_t i = 0; i < 5; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint32_t v = Dencode_varint<uint32_t>(buff);

	return v;
}

uint64_t ByteArray::readVarUint64() {
	uint8_t buff[10] = {0};
	for(size_t i = 0; i < 10; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint64_t v = Dencode_varint<uint64_t>(buff);

	return v;
}

int8_t ByteArray::readVarInt8() {
	uint8_t buff[2] = {0};
	for(size_t i = 0; i < 2; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint8_t v = Dencode_varint<uint8_t>(buff);

	return Dencode_zigzag(v);
}

int16_t ByteArray::readVarInt16() {
	uint8_t buff[3] = {0};
	for(size_t i = 0; i < 3; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint16_t v = Dencode_varint<uint16_t>(buff);

	return Dencode_zigzag(v);
}

int32_t ByteArray::readVarInt32() {
	uint8_t buff[5] = {0};
	for(size_t i = 0; i < 5; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint32_t v = Dencode_varint<uint32_t>(buff);

	return Dencode_zigzag(v);
}

int64_t ByteArray::readVarInt64() {
	uint8_t buff[10] = {0};
	for(size_t i = 0; i < 10; ++i) {
		readFromBlock(&buff[i], 1);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint64_t v = Dencode_varint<uint64_t>(buff);

	return Dencode_zigzag(v);
}

float ByteArray::readFloat() {
	float v = 0;
	readFromBlock(&v, sizeof(float));
	return v;
}

double ByteArray::readDouble() {
	double v = 0;
	readFromBlock(&v, sizeof(double));
	return v;
}

std::string ByteArray::readString() {
	std::string str;
	while(true) {
		uint8_t buff = 0;
		readFromBlock(&buff, 1);
		if(!buff) {
			break;
		}
		str += buff;
	}
	return str;
}

std::string ByteArray::readStringWithLength() {
	size_t len = 0;
	readFromBlock(&len, sizeof(size_t));
	std::string str;
	str.resize(len);
	readFromBlock(&str[0], len);

	return str;
}

// other related
void ByteArray::addCapacity(uint32_t mul) {
	if(!m_isInit) {
		m_dataBlocks.push_back(DataBlock(m_baseSize));
		m_readIterator = m_dataBlocks.begin();
		m_writeIterator = m_dataBlocks.begin();
		m_capacity += m_baseSize;
		--mul;
		m_isInit = true;
	}
	for(uint32_t i = 0; i < mul; ++i) {
		m_dataBlocks.push_back(DataBlock(m_baseSize));
		m_capacity += m_baseSize;
	}
}

void ByteArray::shrink_to_fit() {
	if(!m_capacity) {
		m_dataBlocks.clear();
		m_capacity = 0;
		m_isInit = false;
		return;
	}
	Iterator tmp_writeIterator = m_writeIterator;
	m_dataBlocks.erase(++tmp_writeIterator, m_dataBlocks.end());
	m_capacity = m_dataBlocks.size() * m_baseSize;
}

void ByteArray::clear() {
	m_dataSize = 0;
	m_readIndex = 0;
	m_writeIndex = 0;
	m_readIterator = m_dataBlocks.begin();
	m_writeIterator = m_dataBlocks.begin();
	m_capacity = m_dataBlocks.size() * m_baseSize;
}

}	// namespace webserver
