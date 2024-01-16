//
// Created by YTCCC on 2024/1/4.
//

#include "bytearray.h"
#include "log.h"
#include "util-endian.h"
#include <fstream>
#include <iomanip>
static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace ytccc {
ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}
ByteArray::Node::Node() : ptr(nullptr), next(nullptr), size(0) {}
ByteArray::Node::~Node() {
    delete[] ptr;// 删除数组
}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0),
      m_endian(SYLAR_BIG_ENDIAN), m_root(new Node(base_size)), m_cur(m_root) {
}// base_size 链表长度
ByteArray::~ByteArray() {
    Node *tmp = m_root;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

//write
// 固定长度
void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }
void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }
void ByteArray::writeFint16(int16_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFuint16(uint16_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFint32(int32_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFuint32(uint32_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFint64(int64_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFuint64(uint64_t value) {
    if (m_endian != SYLAR_BYTE_ORDER) value = byteswap(value);
    write(&value, sizeof(value));
}
// 可变长度
static uint32_t EncodeZigzag32(const int32_t &v) {
    // 转变成无符号
    if (v < 0) {// -1变成1
        return ((uint32_t) (-v) * 2) - 1;
    } else {// 1变成2
        return v * 2;
    }
}
static uint64_t EncodeZigzag64(const int64_t &v) {
    if (v < 0) {// -1变成1
        return ((uint64_t) (-v) * 2) - 1;
    } else {// 1变成2
        return v * 2;
    }
}
static int32_t DecodeZigzag32(const uint32_t &v) {
    // 从无符号转换成
    return (int32_t) ((v >> 1) ^ -(v & 1));
    // 先左移一位将乘2消去，再异或最后一位。
}
static int64_t DecodeZigzag64(const uint64_t &v) {
    return (int32_t) ((v >> 1) ^ -(v & 1));
    // 先左移一位将乘2消去，再异或最后一位。
}
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));
}
void ByteArray::writeUint32(uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while (value >= 0x80) {// 0x80 二进制为1000 0000
        tmp[i++] = (value & 0x7f) | 0x80;
        // 0x7f:0111 1111，与上0x7f取低七位数字，或上0x80，添加标记位
        value >>= 7;// 右移七位，处理更高位数字
    }//8位二进制，最高位表示是否有后续字节，其余7位存储数据。
    tmp[i++] = value; // 将剩余不足8位数字存入
    write(tmp, i);
}
void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}
void ByteArray::writeUint64(uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}
void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcmp(&v, &value, sizeof(value));
    writeFuint32(v);
}
void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcmp(&v, &value, sizeof(value));
    writeFuint64(v);
}
// length:int16,data
void ByteArray::writeStringF16(const std::string &value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}
// length:int32,data
void ByteArray::writeStringF32(const std::string &value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}
// length:int64,data
void ByteArray::writeStringF64(const std::string &value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}
// length:varint,data
void ByteArray::writeStringVint(const std::string &value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}
// data
void ByteArray::writeStringWithoutLength(const std::string &value) {
    write(value.c_str(), value.size());
}

//read
int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}
uint8_t ByteArray::readFuint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type)                                                               \
    type v;                                                                    \
    read(&v, sizeof(v));                                                       \
    if (m_endian == SYLAR_BYTE_ORDER) {                                        \
        return v;                                                              \
    } else {                                                                   \
        return byteswap(v);                                                    \
    }

int16_t ByteArray::readFint16() { XX(int16_t); }
uint16_t ByteArray::readFuint16() { XX(uint16_t); }
int32_t ByteArray::readFint32() { XX(int32_t); }
uint32_t ByteArray::readFuint32() { XX(uint32_t); }
int64_t ByteArray::readFint64() { XX(int64_t); }
uint64_t ByteArray::readFuint64() { XX(uint64_t); }
#undef XX
int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for (int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if (b < 0x80) {
            result |= ((uint32_t) b) << i;
            break;
        } else {
            result |= (((uint32_t) (b & 0x7f)) << i);
        }
    }
    return result;
}
int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for (int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if (b < 0x80) {
            result |= ((uint64_t) b) << i;
            break;
        } else {
            result |= (((uint64_t) (b & 0x7f)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
// length:int16,data
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
// length:int32,data
std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
// length:int64,data
std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
// length:varint,data
std::string ByteArray::readStringVint() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

// 内部操作
void ByteArray::clear() {// 只留一个节点，释放其他节点。
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node *tmp = m_root->next;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}
void ByteArray::write(const void *buf, size_t size) {
    if (size == 0) return;                 // check
    addCapacity(size);                     // check
    size_t n_pos = m_position % m_baseSize;// 块内位置
    size_t n_cap = m_cur->size - n_pos;    // 块内容量
    size_t b_pos = 0;                      // 数据偏移

    while (size > 0) {
        if (n_cap >= size) {// 块内容量大于数据大小，不需要其他块
            memcpy(m_cur->ptr + n_pos, (const char *) buf + b_pos, size);
            if (m_cur->size == (n_pos + size)) {
                m_cur = m_cur->next;
            }                  // 当前块已满
            m_position += size;// 位置变动
            b_pos += size;
            size = 0;
        } else {// 块容量小于数据大小
            memcpy(m_cur->ptr + n_pos, (const char *) buf + b_pos, n_cap);
            m_position += n_cap;
            b_pos += n_cap;
            size -= n_cap;
            m_cur = m_cur->next;
            n_cap = m_cur->size;
            n_pos = 0;
            // n_cap = 0;
        }
    }
    if (m_position > m_size) { m_size = m_position; }
}
void ByteArray::read(void *buf, size_t size) {
    // getReadSize():m_size - m_position
    if (size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }                                      //没有这么长的数据
    size_t n_pos = m_position % m_baseSize;// 块内位置
    size_t n_cap = m_cur->size - n_pos;    // 块内容量
    size_t b_pos = 0;
    while (size > 0) {
        if (n_cap >= size) {
            memcpy((char *) buf + b_pos, m_cur->ptr + n_pos, size);
            if (m_cur->size == (n_pos + size)) { m_cur = m_cur->next; }
            m_position += size;
            b_pos += size;
            size = 0;
        } else {
            memcpy((char *) buf + b_pos, m_cur->ptr + n_pos, n_cap);
            m_position += n_cap;
            b_pos += n_cap;
            size -= n_cap;
            m_cur = m_cur->next;
            n_cap = m_cur->size;
            n_pos = 0;
        }
    }
    // read(buf, size, m_position);
}
void ByteArray::read(void *buf, size_t size, size_t position) const {
    if (size > (m_size - position)) {
        throw std::out_of_range("not enough len data");
    }
    size_t n_pos = position % m_baseSize;
    size_t n_cap = m_cur->size - n_pos;
    size_t b_pos = 0;
    Node *cur = m_cur;
    while (size > 0) {
        if (n_cap >= size) {
            memcpy((char *) buf + b_pos, cur->ptr + n_pos, size);
            if (cur->size == (n_pos + size)) { cur = cur->next; }
            position += size;
            b_pos += size;
            size = 0;

        } else {
            memcpy((char *) buf + b_pos, cur->ptr + n_pos, n_cap);
            position += n_cap;
            b_pos += n_cap;
            size -= n_cap;
            cur = cur->next;
            n_cap = cur->size;
            n_pos = 0;
        }
    }
}
void ByteArray::setPosition(size_t v) {
    if (v > m_size) throw std::out_of_range("set position out of range");
    m_position = v;
    m_cur = m_root;
    while (v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if (v == m_cur->size) { m_cur = m_cur->next; }
}

bool ByteArray::writeToFile(const std::string &name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);//删除内容并打开
    if (!ofs) {
        SYLAR_LOG_ERROR(g_logger)
            << "writeToFile name=" << name << " error, error=" << errno
            << " err_str=" << strerror(errno);
        return false;
    }
    auto read_size = (int64_t) getReadSize();// 当前位置开始到结束数据的长度。
    int64_t pos = m_position;
    Node *cur = m_cur;
    while (read_size > 0) {
        int diff = pos % m_baseSize;// 数据偏移
        int64_t len = (int64_t) (read_size > (int64_t) m_baseSize ? m_baseSize
                                                                  : read_size) -
                      diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }
    // SYLAR_LOG_INFO(g_logger) << "writeToFile success";
    return true;
}
bool ByteArray::readFromFile(const std::string &name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if (!ifs) {
        SYLAR_LOG_ERROR(g_logger)
            << "readFromFile name=" << name << " error, error=" << errno
            << " err_str=" << strerror(errno);
        return false;
    }
    std::shared_ptr<char> buff(new char[m_baseSize],
                               [](char *ptr) { delete[] ptr; });
    while (!ifs.eof()) {                 // 文件流是否到达文件的末尾
        ifs.read(buff.get(), m_baseSize);// 读取指定大小数据到字符数组中
        write(buff.get(), ifs.gcount());// 将读取的数据写入链表中
    }
    // SYLAR_LOG_INFO(g_logger) << "readFromFile success";
    return true;
}

bool ByteArray::isLittleEndian() const {
    return m_endian == SYLAR_LITTLE_ENDIAN;
}
bool ByteArray::setIsLittleEndian(bool val) {
    if (val) {
        m_endian = SYLAR_LITTLE_ENDIAN;
    } else {
        m_endian = SYLAR_BIG_ENDIAN;
    }
    return true;
}

std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if (str.empty()) return str;
    read(&str[0], str.size(), m_position);
    // SYLAR_LOG_INFO(g_logger) << "toString success";
    return str;
}
std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;
    for (size_t i = 0; i < str.size(); ++i) {
        if (i > 0 && i % 32 == 0) { ss << std::endl; }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int) (uint8_t) str[i] << " ";
    }
    return ss.str();
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) return 0;

    uint64_t size = len;// 实际读取长度

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov {};
    Node *cur = m_cur;

    while (len > 0) {
        if (ncap >= m_cur->size) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) return 0;

    uint64_t size = len;// 实际读取长度
    size_t count = position / m_baseSize;
    Node *cur = m_root;
    while (count > 0) {
        cur = cur->next;
        --count;
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov {};
    while (len > 0) {
        if (ncap >= m_cur->size) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}
uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
    if (len == 0) return 0;
    addCapacity(len);
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov {};
    Node *cur = m_cur;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return len;
}
void ByteArray::addCapacity(size_t size) {
    size_t old_cap = getCapacity();// 剩余容量
    if (size == 0 || old_cap >= size) return;// check 剩余容量够用，不用重新增加
    size -= old_cap;                         // 超出范围大小
    size_t count =
        ceil((double) size / (double) m_baseSize);// 向上取整，应该是所需块数量
    // size_t count =
    //     (size / m_baseSize) + ((size % m_baseSize) > old_cap ? 1 : 0);
    // count 1：
    Node *tmp = m_root;
    while (tmp->next) { tmp = tmp->next; }// 末尾节点
    Node *first = nullptr;
    for (size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if (first == nullptr) first = tmp->next;// 若为空，
        tmp = tmp->next;
        m_capacity += m_baseSize;// 已有数据增加
    }
    if (old_cap == 0) { m_cur = first; }// 之前已经用完的情况下，可能会指向空
}

}// namespace ytccc