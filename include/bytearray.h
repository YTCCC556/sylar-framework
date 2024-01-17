//
// Created by YTCCC on 2024/1/4.
//

#ifndef YTCCC_MODULE_BYTEARRAY_H
#define YTCCC_MODULE_BYTEARRAY_H

#include <cstdint>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <vector>

namespace ytccc {
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    struct Node {
        Node(size_t s);//新建指针指向字符类型，初始化next指针，大小
        Node();        // 指针指向空，其余同上
        ~Node();       // 释放指针

        char *ptr;
        size_t size;
        Node *next;
    };

    ByteArray(size_t base_size = 4096);
    // base_size 链表长度,初始化数据，块大小，块数量，数据数量，当前位置，根块指针，当前块指针
    ~ByteArray();// 从根块开始释放所有块指针。

    //write
    // 固定长度
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);
    // 可变长度 进行特殊编码
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);
    // length:int16,data
    void writeStringF16(const std::string &value);
    // length:int32,data
    void writeStringF32(const std::string &value);
    // length:int64,data
    void writeStringF64(const std::string &value);
    // length:varint,data
    void writeStringVint(const std::string &value);
    // data
    void writeStringWithoutLength(const std::string &value);

    //read
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();
    // length:int16,data
    std::string readStringF16();
    // length:int32,data
    std::string readStringF32();
    // length:int64,data
    std::string readStringF64();
    // length:varint,data
    std::string readStringVint();

    // 内部操作
    void clear();// 保留根节点，去除其他节点。
    void write(const void *buf, size_t size);// 把数据写入链表中
    void read(void *buf, size_t size);       // 读取size个数据到buf中
    void read(void *buf, size_t size, size_t position) const;
    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);// 设置当前位置以及当前指针

    bool writeToFile(const std::string &name)
        const;// 将从当前位置开始到末尾结束的数据写入文件
    bool readFromFile(const std::string &name);//

    size_t getBaseSize() const { return m_baseSize; }
    size_t getReadSize() const { return m_size - m_position; }
    size_t getSize() const { return m_size; }
    bool isLittleEndian() const;
    bool setIsLittleEndian(bool val);

    std::string toString() const;// 调用read函数
    std::string toHexString() const;

    uint64_t getReadBuffers(std::vector<iovec> &buffers,
                            uint64_t len = ~0) const;
    uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                            uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

private:
    void
    addCapacity(size_t size);// 增加容量，根据数据大小，增加块数量。注意容量位置
    size_t getCapacity() const { return m_capacity - m_position; }// 剩余容量

private:
    size_t m_baseSize;// 内存块大小
    size_t m_size;    // 当前数据大小
    size_t m_position;// 操作位置
    size_t m_capacity;// 已有数据大小
    int m_endian;     // 大小端
    Node *m_root;     // 第一个内存块指针
    Node *m_cur;      // 当前内存块指针
};
}// namespace ytccc


#endif//YTCCC_MODULE_BYTEARRAY_H
