//
// Created by YTCCC on 2024/1/8.
//
#include "bytearray.h"
#include "log.h"
#include "macro.h"

static ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test() {
    /*    // auto type = int8_t ;
    std::vector<int8_t> vec;
    for (int i = 0; i < 100; ++i) { vec.push_back(rand()); }
    ytccc::ByteArray::ptr ba(new ytccc::ByteArray(1));
    for (auto &i: vec) { ba->writeFint8(i); }
    */
    /*SYLAR_LOG_INFO(g_logger) << #write_fun;  */      /*
    ba->setPosition(0);
    for (size_t i = 0; i < vec.size(); ++i) {
        */
    /*SYLAR_LOG_INFO(g_logger) << #read_fun;        */ /*
        int8_t v = ba->readFint8();
        SYLAR_LOG_INFO(g_logger) << i << " - " << v << " - " << vec[i]
                                 << " - " << (int32_t) vec[i];
        SYLAR_ASSERT(v == vec[i]);
    }
    SYLAR_ASSERT(ba->getReadSize() == 0);
    SYLAR_LOG_INFO(g_logger)
        <<  "writeFint8/readFint8"  " (int8_t) len=" << 100
        << " base=" << 1 << " size=" << ba->getSize();*/

#define XX(type, len, write_fun, read_fun, base_len)                           \
    {                                                                          \
        std::vector<type> vec;                                                 \
        for (int i = 0; i < len; ++i) { vec.push_back(rand()); }               \
        ytccc::ByteArray::ptr ba(new ytccc::ByteArray(base_len));              \
        for (auto &i: vec) { ba->write_fun(i); } /*将数据存入链表中*/  \
        /*SYLAR_LOG_INFO(g_logger) << #write_fun; */                           \
        ba->setPosition(0);                                                    \
        /*SYLAR_LOG_INFO(g_logger) << #read_fun;   */                          \
        for (size_t i = 0; i < vec.size(); ++i) {                              \
            type v = ba->read_fun(); /*从链表中共读取出数据*/        \
            SYLAR_LOG_INFO(g_logger)                                           \
                << i << " - " << v << " - " << (int32_t) vec[i];               \
            SYLAR_ASSERT(v == vec[i]);                                         \
        }                                                                      \
        SYLAR_ASSERT(ba->getReadSize() == 0);                                  \
        SYLAR_LOG_INFO(g_logger)                                               \
            << #write_fun "/" #read_fun " (" #type ") len=" << len             \
            << " base=" << base_len << " size=" << ba->getSize();              \
    }

    // XX(int8_t, 5, writeFint8, readFint8, 1);
    // XX(uint8_t, 5, writeFuint8, readFuint8, 1);
    // XX(int16_t, 100, writeFint16, readFint16, 1);
    // XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    // XX(int32_t, 100, writeFint32, readFint32, 1);
    // XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    // XX(int64_t, 100, writeFint64, readFint64, 1);
    // XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 5, writeInt32, readInt32, 1);
    // XX(uint32_t, 100, writeUint32, readUint32, 1);
    // XX(int64_t, 100, writeInt64, readInt64, 1);
    // XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

#define XX(type, len, write_fun, read_fun, base_len)                           \
    {                                                                          \
        std::vector<type> vec;                                                 \
        for (int i = 0; i < len; ++i) { vec.push_back(rand()); }               \
        ytccc::ByteArray::ptr ba(new ytccc::ByteArray(base_len));              \
        for (auto &i: vec) { ba->write_fun(i); }                               \
        ba->setPosition(0);                                                    \
        for (size_t i = 0; i < vec.size(); ++i) {                              \
            type v = ba->read_fun();                                           \
            SYLAR_ASSERT(v == vec[i]);                                         \
        }                                                                      \
        SYLAR_ASSERT(ba->getReadSize() == 0);                                  \
        SYLAR_LOG_INFO(g_logger)                                               \
            << #write_fun "/" #read_fun " (" #type ") len=" << len             \
            << " base=" << base_len << " size=" << ba->getSize();              \
        ba->setPosition(0);                                                    \
        SYLAR_ASSERT(                                                          \
            ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));     \
        ytccc::ByteArray::ptr ba2(new ytccc::ByteArray(base_len * 2));         \
        SYLAR_ASSERT(                                                          \
            ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));   \
        ba2->setPosition(0);                                                   \
        SYLAR_ASSERT(ba->toString() == ba2->toString());                       \
        SYLAR_ASSERT(ba->getPosition() == 0);                                  \
        SYLAR_ASSERT(ba2->getPosition() == 0);                                 \
    }

    // XX(int8_t, 100, writeFint8, readFint8, 1);
    // XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    // XX(int16_t, 100, writeFint16, readFint16, 1);
    // XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    // XX(int32_t, 100, writeFint32, readFint32, 1);
    // XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    // XX(int64_t, 100, writeFint64, readFint64, 1);
    // XX(uint64_t, 100, writeFuint64, readFuint64, 1);
    //
    // XX(int32_t, 100, writeInt32, readInt32, 1);
    // XX(uint32_t, 100, writeUint32, readUint32, 1);
    // XX(int64_t, 100, writeInt64, readInt64, 1);
    // XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

int main(int argc, char **argv) {
    test();
    return 0;
}