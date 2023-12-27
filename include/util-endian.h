//
// Created by YTCCC on 2023/12/25.
//

#ifndef YTCCC_MODULE_UTIL_ENDIAN_H
#define YTCCC_MODULE_UTIL_ENDIAN_H
#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2

#include <boost/asio.hpp>
#include <byteswap.h>
#include <stdint.h>
namespace ytccc {
uint32_t convertToLittleEndianIPv4(const char *ipAddress) {
    boost::asio::ip::address_v4 addr =
            boost::asio::ip::address_v4::from_string(ipAddress);
    // 将IPv4地址转换为32位整数，并进行字节序转换
    return boost::asio::detail::socket_ops::host_to_network_long(
            addr.to_ulong());
}
uint32_t convertToLittleEndianIPv4(uint32_t ipAddress) {
    // 将IPv4地址进行字节序转换
    return boost::asio::detail::socket_ops::host_to_network_long(ipAddress);
}
uint32_t convertToLittleEndianPort(uint32_t port) {
    return boost::asio::detail::socket_ops::host_to_network_short(port);
}
/**
 * @brief 8字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T) bswap_64((uint64_t) value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T) bswap_32((uint32_t) value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T) bswap_16((uint16_t) value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}// namespace ytccc
#endif//YTCCC_MODULE_UTIL_ENDIAN_H
