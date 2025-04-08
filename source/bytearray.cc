#include "bytearray.h"
#include "macro.h"

#include <type_traits>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace webserver {

// ByteArray::DataBlock
ByteArray::DataBlock::DataBlock(uint64_t baseSize)
	: data(nullptr) {
	data = (uint8_t*)malloc(baseSize);
}

ByteArray::DataBlock::DataBlock(DataBlock&& dataBlock) {
	data = dataBlock.data;
	dataBlock.data = nullptr;
}

ByteArray::DataBlock::~DataBlock() {
	if(data) {
		free(data);
	}
}

// ByteArray
ByteArray::ByteArray(uint64_t baseSize)
	: m_isInit(false)
	, m_fail(false)
	, m_baseSize(baseSize)
	, m_dataSize(0), m_capacity(0)
	, m_readIndex(0), m_writeIndex(0)
	, m_readIterator{}, m_writeIterator{}
	, m_dataBlocks{} {
}
	
ByteArray::~ByteArray() = default;

void ByteArray::writeToBlock(const void* buff, uint64_t len) {
	if(m_fail) {
		return;
	}
	while(m_capacity <= len + m_dataSize) {
		m_dataBlocks.emplace_back(m_baseSize);
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
		uint64_t left_size = m_baseSize - m_writeIndex;
	
		if(left_size > len) {
			memcpy(data_block + m_writeIndex, buff_ptr, len);	
			m_writeIndex += len;
			m_dataSize += len;
			break;
		} else {
			memcpy(data_block + m_writeIndex, buff_ptr, left_size);
			++m_writeIterator;
			m_writeIndex = 0;
			m_dataSize += left_size;
			buff_ptr += left_size;
			len -= left_size;
		}
	}
}

void ByteArray::readFromBlock(void* buff, uint64_t len) {
	if(m_fail) {
		return;
	}
	WEBSERVER_ASSERT2(len <= m_dataSize, "readFromBlock len is too long");
	
	uint8_t* buff_ptr = static_cast<uint8_t*>(buff);
	
	while(len > 0) {
		uint8_t* data_block = m_readIterator->data;
		uint64_t data_size = m_baseSize - m_readIndex;

		if(data_size > len) {
			memcpy(buff_ptr, data_block + m_readIndex, len);
			m_readIndex += len;
			m_dataSize -= len;
			m_capacity -= len;
			break;
		} else {
			memcpy(buff_ptr, data_block + m_readIndex, data_size);
			m_dataSize -= data_size;
			m_capacity -= data_size;
			if(m_readIterator != m_writeIterator) {
				++m_readIterator;
				m_readIndex = 0;
			} else {
				m_readIndex += data_size;
				WEBSERVER_ASSERT(m_readIndex == m_writeIndex);
			}
			len -= data_size;
			buff_ptr += data_size;
		}
	}
	for(auto it = m_dataBlocks.begin(); it != m_readIterator; ++it) {
		m_dataBlocks.push_back(std::move(*it));
		m_capacity += m_baseSize;
	}
	m_dataBlocks.erase(m_dataBlocks.begin(), m_readIterator);
}

void ByteArray::peekFromBlock(void* buff, uint64_t len, uint64_t offset) const {
	if(m_fail) {
		return;
	}
	WEBSERVER_ASSERT2(len + offset <= m_dataSize, "peekFromBlock len is too long");
	
	uint8_t* buff_ptr = static_cast<uint8_t*>(buff);
	uint64_t readIndex = (m_readIndex + offset) % m_baseSize; 
	Iterator readIterator = m_readIterator;
	for(size_t i = 0; i < (m_readIndex + offset) / m_baseSize; ++i) {
		++readIterator;
	}
	
	while(len > 0) {
		uint8_t* data_block = readIterator->data;
		uint64_t data_size = m_baseSize - readIndex;
		if(data_size > len) {
			memcpy(buff_ptr, data_block + readIndex, len);
			break;
		} else {
			memcpy(buff_ptr, data_block + readIndex, data_size);
			if(readIterator != m_writeIterator) {
				++readIterator;
				readIndex = 0;
			} else {
				readIndex += data_size;
				WEBSERVER_ASSERT(m_readIndex == m_writeIndex);
			}
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
	buff[len++] = v;
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

void ByteArray::writeStringAsC(const std::string& str) {
	writeToBlock(str.c_str(), str.size() + 1);
}

void ByteArray::writeString(const std::string& str, uint64_t len) {
	if(len > str.size()) {
		throw std::out_of_range("ByteArray::writeString len out of range");
	}
	writeToBlock(str.c_str(), len);
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

std::string ByteArray::readStringAsC() {
	std::string str;
	while(true) {
		uint8_t buff = 0;
		readFromBlock(&buff, 1);
		str += buff;
		if(!buff) {
			break;
		}
	}
	return str;
}

std::string ByteArray::readString(uint64_t len) {
	std::string str;
	str.resize(len);
	readFromBlock(&str[0], len);
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

// peek related
uint8_t ByteArray::peekFixUint8() const {
	uint8_t v = 0;
	peekFromBlock(&v, sizeof(uint8_t), 0);
	return v;
}

uint16_t ByteArray::peekFixUint16() const {
	uint16_t v = 0;
	peekFromBlock(&v, sizeof(uint16_t), 0);
	return v;
}

uint32_t ByteArray::peekFixUint32() const {
	uint32_t v = 0;
	peekFromBlock(&v, sizeof(uint32_t), 0);
	return v;
}

uint64_t ByteArray::peekFixUint64() const {
	uint64_t v = 0;
	peekFromBlock(&v, sizeof(uint64_t), 0);
	return v;
}

int8_t ByteArray::peekFixInt8() const {
	int8_t v = 0;
	peekFromBlock(&v, sizeof(int8_t), 0);
	return v;
}

int16_t ByteArray::peekFixInt16() const {
	int16_t v = 0;
	peekFromBlock(&v, sizeof(int16_t), 0);
	return v;
}

int32_t ByteArray::peekFixInt32() const {
	int32_t v = 0;
	peekFromBlock(&v, sizeof(int32_t), 0);
	return v;
}

int64_t ByteArray::peekFixInt64() const {
	int64_t v = 0;
	peekFromBlock(&v, sizeof(int64_t), 0);
	return v;
}

uint8_t ByteArray::peekVarUint8() const {
	uint8_t buff[2] = {0};
	for(size_t i = 0; i < 2; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint8_t v = Dencode_varint<uint8_t>(buff);

	return v;
}

uint16_t ByteArray::peekVarUint16() const {
	uint8_t buff[3] = {0};
	for(size_t i = 0; i < 3; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint16_t v = Dencode_varint<uint16_t>(buff);

	return v;
}

uint32_t ByteArray::peekVarUint32() const {
	uint8_t buff[5] = {0};
	for(size_t i = 0; i < 5; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint32_t v = Dencode_varint<uint32_t>(buff);

	return v;
}

uint64_t ByteArray::peekVarUint64() const {
	uint8_t buff[10] = {0};
	for(size_t i = 0; i < 10; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint64_t v = Dencode_varint<uint64_t>(buff);

	return v;
}

int8_t ByteArray::peekVarInt8() const {
	uint8_t buff[2] = {0};
	for(size_t i = 0; i < 2; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint8_t v = Dencode_varint<uint8_t>(buff);

	return Dencode_zigzag(v);
}

int16_t ByteArray::peekVarInt16() const {
	uint8_t buff[3] = {0};
	for(size_t i = 0; i < 3; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint16_t v = Dencode_varint<uint16_t>(buff);

	return Dencode_zigzag(v);
}

int32_t ByteArray::peekVarInt32() const {
	uint8_t buff[5] = {0};
	for(size_t i = 0; i < 5; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint32_t v = Dencode_varint<uint32_t>(buff);

	return Dencode_zigzag(v);
}

int64_t ByteArray::peekVarInt64() const {
	uint8_t buff[10] = {0};
	for(size_t i = 0; i < 10; ++i) {
		peekFromBlock(&buff[i], 1, i);
		if(!(buff[i] & 0x80)) {
			break;
		}
	}
	uint64_t v = Dencode_varint<uint64_t>(buff);

	return Dencode_zigzag(v);
}

float ByteArray::peekFloat() const {
	float v = 0;
	peekFromBlock(&v, sizeof(float), 0);
	return v;
}

double ByteArray::peekDouble() const {
	double v = 0;
	peekFromBlock(&v, sizeof(double), 0);
	return v;
}

std::string ByteArray::peekStringAsC() const {
	std::string str;
	uint64_t i = 0;
	while(true) {
		uint8_t buff = 0;
		peekFromBlock(&buff, 1, i++);
		str += buff;
		if(!buff) {
			break;
		}
	}
	return str;
}

std::string ByteArray::peekString(uint64_t len) const {
	std::string str;
	str.resize(len);
	peekFromBlock(&str[0], len, 0);
	return str;
}

std::string ByteArray::peekStringWithLength() const {
	size_t len = 0;
	peekFromBlock(&len, sizeof(size_t), 0);
	std::string str;
	str.resize(len);
	peekFromBlock(&str[0], len, sizeof(size_t));

	return str;
}

// other related
void ByteArray::addCapacity(uint32_t mul) {
	if(m_fail) {
		return;
	}
	if(!m_isInit) {
		m_dataBlocks.emplace_back(m_baseSize);
		m_readIterator = m_dataBlocks.begin();
		m_writeIterator = m_dataBlocks.begin();
		m_capacity += m_baseSize;
		--mul;
		m_isInit = true;
	}
	for(uint32_t i = 0; i < mul; ++i) {
		m_dataBlocks.emplace_back(m_baseSize);
		m_capacity += m_baseSize;
	}
}

void ByteArray::shrink_to_fit() {
	if(m_fail) {
		return;
	}
	if(!m_dataSize) {
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
	if(m_fail || !m_isInit) {
		return;
	}
	m_dataSize = 0;
	m_readIndex = 0;
	m_writeIndex = 0;
	m_readIterator = m_dataBlocks.begin();
	m_writeIterator = m_dataBlocks.begin();
	m_capacity = m_dataBlocks.size() * m_baseSize;
}

uint64_t ByteArray::getBaseSize() const { 
	if(m_fail) {
		return 0;
	}
	return m_baseSize; 
}

uint64_t ByteArray::getDataSize() const { 
	if(m_fail) {
		return 0;
	}
	return m_dataSize; 
}

uint64_t ByteArray::getCapacity() const { 
	if(m_fail) {
		return 0;
	}
	return m_capacity; 
}

bool ByteArray::writeToFile(const std::string& path) {
	if(m_fail) {
		return false;
	}
	std::ofstream ofs;
	ofs.open(path, std::ios::binary | std::ios::trunc);
	if(!ofs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::writeToFile -- open file error : " + path);
		return false;
	}
	uint64_t data_size = m_dataSize;
	uint8_t* buff = (uint8_t*)malloc(data_size);
	readFromBlock(buff, data_size);
	if(!ofs.write((char*)buff, data_size)) {
		LIBFUN_STD_LOG(errno, "ByteArray::writeFromFile -- write file error : " + path);
		return false;
	}
	free(buff);
	
	ofs.close();
	return true;
}

bool ByteArray::readFromFile(const std::string& path) {
	if(m_fail) {
		return false;
	}
	std::ifstream ifs;
	ifs.open(path, std::ios::binary);
	if(!ifs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::readFromFile -- open file error : " + path);
		return false;
	}
	
	ifs.seekg(0, std::ios_base::end);
	size_t file_size = ifs.tellg();
	ifs.seekg(0, std::ios_base::beg);

	uint8_t* buff = (uint8_t*)malloc(file_size);
	if(!ifs.read((char*)buff, file_size)) {
		LIBFUN_STD_LOG(errno, "ByteArray::readFromFile -- read file error : " + path);
		return false;
	}
	writeToBlock(buff, ifs.gcount());
	free(buff);
	
	ifs.close();
	return true;
}

bool ByteArray::peekToFile(const std::string& path) const {
	if(m_fail) {
		return false;
	}
	std::ofstream ofs;
	ofs.open(path, std::ios::binary | std::ios::trunc);
	if(!ofs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::peekToFile -- open file error : " + path);
		return false;
	}
	uint64_t data_size = m_dataSize;
	uint8_t* buff = (uint8_t*)malloc(data_size);
	peekFromBlock(buff, data_size, 0);
	if(!ofs.write((char*)buff, data_size)) {
		LIBFUN_STD_LOG(errno, "ByteArray::peekFromFile -- peek to file error : " + path);
		return false;
	}
	free(buff);
	
	ofs.close();
	return true;
}

bool ByteArray::writeToFileHex(const std::string& path) {
	if(m_fail) {
		return false;
	}
	std::ofstream ofs;
	ofs.open(path, std::ios::trunc);
	if(!ofs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::writeToFileHex -- open file error : " + path);
		return false;
	}

	ofs << std::hex;
	uint64_t data_size = m_dataSize;
	uint8_t* buff = (uint8_t*)malloc(data_size);
	readFromBlock(buff, data_size);
	for(size_t i = 0; i < data_size; ++i) {
		ofs << "0x" << std::setw(2) << std::setfill('0') 
			<< (uint32_t)buff[i];
		if(i != data_size - 1) {
			ofs << " ";
		}
	}
	free(buff);
	
	ofs.close();
	return true;
}

bool ByteArray::readFromFileHex(const std::string& path) {
	if(m_fail) {
		return false;
	}
	std::ifstream ifs;
	ifs.open(path);
	if(!ifs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::readToFileHex -- open file error : " + path);
		return false;
	}
	
	std::string input;
	while(ifs >> input) {
		uint32_t v = 0;
		std::istringstream iss(input.substr(2));
		iss >> std::hex >> v;
		uint8_t data = static_cast<uint8_t>(v);
		writeToBlock(&data, sizeof(uint8_t));
	}
		
	ifs.close();
	return true;
}

bool ByteArray::peekToFileHex(const std::string& path) const {
	if(m_fail) {
		return false;
	}
	std::ofstream ofs;
	ofs.open(path, std::ios::trunc);
	if(!ofs.is_open()) {
		LIBFUN_STD_LOG(errno, "ByteArray::peekToFileHex -- open file error : " + path);
		return false;
	}

	ofs << std::hex;
	uint64_t data_size = m_dataSize;
	uint8_t* buff = (uint8_t*)malloc(data_size);
	peekFromBlock(buff, data_size, 0);

	for(size_t i = 0; i < data_size; ++i) {
		ofs << "0x" << std::setw(2) << std::setfill('0') 
			<< (uint32_t)buff[i];
		if(i != data_size - 1) {
			ofs << " ";
		}
	}
	free(buff);
	
	ofs.close();
	return true;
}

bool ByteArray::getReadBuffers(std::vector<struct iovec>& iovecs, uint64_t len) {
	if(m_dataSize < len || m_fail) {
		return false;
	}

	Iterator readIterator = m_readIterator;
	uint64_t readIndex = m_readIndex;
	
	while(len > 0) {
		uint64_t data_size = readIterator == m_writeIterator ? 
				m_writeIndex - readIndex : m_baseSize - readIndex;
		
		if(data_size > len) {
			struct iovec io_vec;
			io_vec.iov_base = readIterator->data + readIndex;
			io_vec.iov_len = len;
			iovecs.push_back(io_vec);
			break;
		} else {
			struct iovec io_vec;
			io_vec.iov_base = readIterator->data + readIndex;
			io_vec.iov_len = data_size;
			iovecs.push_back(io_vec);
			
			++readIterator;
			readIndex = 0;
			len -= data_size;
		}
	}
	return true;
}

bool ByteArray::getWriteBuffers(std::vector<struct iovec>& iovecs, uint64_t len) {
	if(m_fail) {
		return false;
	}
	while(m_capacity <= m_dataSize + len) {
		addCapacity(1);
	}

	m_fail = true;

	Iterator writeIterator = m_writeIterator;
	uint64_t writeIndex = m_writeIndex;
	
	struct iovec io_vec;
	while(len > 0) {
		uint64_t left_size = m_baseSize - writeIndex;
		if(left_size > len) {
			io_vec.iov_base = writeIterator->data + writeIndex;
			io_vec.iov_len = len;
			iovecs.push_back(io_vec);
			break;
		} else {
			io_vec.iov_base = writeIterator->data + writeIndex;
			io_vec.iov_len = left_size;
			iovecs.push_back(io_vec);
			
			++writeIterator;
			writeIndex = 0;
			len -= left_size;
		}
	}

	return true;
}

bool ByteArray::restorative(uint64_t byteSize) {
	if(!m_fail) {
		return false;
	}
	
	while(byteSize > 0) {
		uint64_t block_left_size = m_baseSize - m_writeIndex;
		if(block_left_size > byteSize) {
			m_writeIndex += byteSize;
			m_capacity -= byteSize;
			m_dataSize += byteSize;
			break;
		} else {
			++m_writeIterator;
			m_writeIndex = 0;	
			m_dataSize += block_left_size;
			byteSize -= block_left_size;
		}
	}
	
	if(m_writeIterator == m_dataBlocks.end()) {
		m_dataBlocks.emplace_back(m_baseSize);
		m_capacity += m_baseSize;
		m_writeIterator = --m_dataBlocks.end();
	}
	
	m_fail = false;
	return true;
}

}	// namespace webserver
