//
// Created by YTCCC on 2023/12/21.
//
#include "address.h"
#include "log.h"
#include "util-endian.h"
#include <cstddef>
#include <cstring>
#include <ifaddrs.h>
#include <memory>
#include <netdb.h>
#include <sstream>

namespace ytccc {

static ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();

template<class T>
static T CreateMask(uint32_t bits) {
    // 返回取反的掩码
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}
template<class T>
static uint32_t CountBytes(T value) {
    // 统计value中有多少1
    uint32_t result = 0;
    for (; value; ++result) { value &= value - 1; }
    return result;
}

// Address
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if (addr == nullptr) { return nullptr; }
    Address::ptr result;
    switch (addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *) addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *) addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}
bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {
    // host中存储了域名或服务器名
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    std::string node;
    const char *service = nullptr;
    // 检查ipv6address service
    if (!host.empty() && host[0] == '[') {// 以'['开头表面为ipv6地址
        // 在host中搜索字符']'，搜索范围为整个host
        const char *endipv6 =
                (const char *) memchr(host.c_str() + 1, ']', host.size() - 1);
        if (endipv6) {
            // TODO check out of range
            if (*(endipv6 + 1) == ':') service = endipv6 + 2;
            // 地址内容为结束位置减去开头位置-1
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }
    // 检查node service 检查端口号
    if (node.empty()) {// host开头不为'['，或[]之间无内容
        service = (const char *) memchr(host.c_str(), ':', host.size());
        if (service) {// host中有':'，service为指向':'的指针，
            if (!memchr(service + 1, ':',
                        host.c_str() + host.size() - service - 1)) {
                // ':'之后没有':'了
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }
    if (node.empty())
        node = host;// 若node还是空的，说明host中不包含'[]'以及':'等附加内容
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger)
                << "Address::Lookup get address(" << host << ", " << family
                << ", " << type << ") error=" << error
                << " err_str=" << gai_strerror(error);
        return false;
    }
    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t) next->ai_addrlen));
        next = next->ai_next;
    }
    freeaddrinfo(results);
    return !result.empty();
}
Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) { return result[0]; }
    return nullptr;
}
IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto &i: result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) return v;
        }
    }
    return nullptr;
}
bool Address::GetInterfaceAddress(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
        int family) {// 获得指定网卡的地址
    struct ifaddrs *next, *results;
    // 获得当前系统中所有网络接口的地址信息，成功返回0，失败返回-1
    if (getifaddrs(&results) != 0) {
        SYLAR_LOG_ERROR(g_logger)
                << "Address::GetInterfaceAddress get if_addrs err=" << errno
                << " err_str=" << strerror(errno);
        return false;
    }
    try {
        for (next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_length = ~0u;// 无符号最大整数
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }//AF_UNSPEC 未指定地址簇
            switch (next->ifa_addr->sa_family) {
                    //根据地址类型创建地址，获得网络掩码，和字符长度
                case AF_INET: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in *) next->ifa_netmask)
                                               ->sin_addr.s_addr;
                    prefix_length = CountBytes(netmask);
                } break;
                case AF_INET6: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr &netmask =
                            ((sockaddr_in6 *) next->ifa_netmask)->sin6_addr;
                    prefix_length = 0;
                    for (int i = 0; i < 16; ++i) {
                        prefix_length += CountBytes(netmask.s6_addr[i]);
                    }
                } break;
                default:
                    break;
            }
            if (addr) {
                result.insert(std::make_pair(
                        next->ifa_name, std::make_pair(addr, prefix_length)));
            }
        }
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return true;
}
bool Address::GetInterfaceAddress(
        std::vector<std::pair<Address::ptr, uint32_t>> &result,
        const std::string &iface, int family) {// 获得指定网卡的地址
    // 指定内容为空，创建地址类返回。
    if (iface.empty() || iface == "*") {
        if (family == AF_INET || family == AF_UNSPEC) {
            result.emplace_back(Address::ptr(new IPv4Address()), 0u);
        }
        if (family == AF_INET6 || family == AF_UNSPEC) {
            result.emplace_back(Address::ptr(new IPv6Address()), 0u);
        }
        return true;
    }
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
    if (!GetInterfaceAddress(results, family)) { return false; }
    auto its = results.equal_range(iface);// 返回一个表示范围的pair对象，
    for (; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return true;
}
int Address::getFamily() const { return getAddr()->sa_family; }
// std::ostream &Address::insert(std::ostream &os) const {}
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}
bool Address::operator<(const Address &rhs) const {
    socklen_t minlen = std::min(getAddrlen(), rhs.getAddrlen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {// 小于rhs
        return true;
    } else if (result > 0) {// 大于rhs
        return false;
    } else if (getAddrlen() < rhs.getAddrlen()) {// 相等比较地址长度
        return true;
    } else {
        return false;
    }
}
bool Address::operator==(const Address &rhs) const {
    return getAddrlen() == rhs.getAddrlen() &&
           memcmp(getAddr(), rhs.getAddr(), getAddrlen()) == 0;
}
bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

// IPAddress
IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    addrinfo hints{}, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_CANONIDN;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, nullptr, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger)
                << "IPAddress::Create(" << address << ", " << port
                << ") error=" << error << " errno=" << errno
                << " err_str=" << strerror(errno);
        return nullptr;
    }
    try {
        IPAddress::ptr result =
                std::dynamic_pointer_cast<IPAddress>(Address::Create(
                        results->ai_addr, (socklen_t) results->ai_addrlen));
        if (results) { result->setPort(port); }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}
// IPv4Address
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    // 将文本地址转化成ip地址
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    // 将address转换为二进制形式存入
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) {// 转换失败，输出日志
        SYLAR_LOG_ERROR(g_logger)
                << "IPv4Address::Create(" << address << "," << port
                << ") rt=" << result << " errno=" << errno
                << " err_str=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in &address) { m_addr = address; }
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));// 置空
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}
const sockaddr *IPv4Address::getAddr() const { return (sockaddr *) &m_addr; }
sockaddr *IPv4Address::getAddr() { return (sockaddr *) &m_addr; }
socklen_t IPv4Address::getAddrlen() const { return sizeof(m_addr); }
std::ostream &IPv4Address::insert(std::ostream &os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "." << ((addr) & 0xff);//将int转换成ipv4地址
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if (prefix_len > 32) { return nullptr; }
    sockaddr_in baddr(m_addr);// 根据m_addr创建新的独立的结构体
    baddr.sin_addr.s_addr |=
            byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));// 掩码操作
    return std::make_shared<IPv4Address>(baddr);
}
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if (prefix_len > 32) { return nullptr; }
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &=
            byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));// 掩码操作
    return std::make_shared<IPv4Address>(baddr);
}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet{};
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr =
            ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(subnet);
}

uint16_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

// IPv6Address
IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
    // 将文本地址转化成ip地址
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if (result <= 0) {// 转换失败，输出日志
        SYLAR_LOG_ERROR(g_logger)
                << "IPv6Address::Create(" << address << "," << port
                << ") rt=" << result << " errno=" << errno
                << " err_str=" << strerror(errno);
        return nullptr;
    }
    return rt;
}
IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));// 置空
    m_addr.sin6_family = AF_INET6;
}
IPv6Address::IPv6Address(const sockaddr_in6 &address) { m_addr = address; }
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));// 置空
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr *IPv6Address::getAddr() const { return (sockaddr *) &m_addr; }
sockaddr *IPv6Address::getAddr() { return (sockaddr *) &m_addr; }
socklen_t IPv6Address::getAddrlen() const { return sizeof(m_addr); }
std::ostream &IPv6Address::insert(std::ostream &os) const {
    os << "[";
    auto *addr = (uint16_t *) m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for (size_t i = 0; i < 8; ++i) {
        // 零压缩法
        if (addr[i] == 0 && !used_zeros) {
            // 当前位为0，且没用过0表示，继续下次循环
            continue;
        }
        if (i && addr[i - 1] == 0 && !used_zeros) {
            // i真，前一个位置为0，并且没有用过0表示
            os << ":";
            used_zeros = true;
        }
        if (i) { os << ":"; }
        os << std::hex << (int) byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if (!used_zeros && addr[7] == 0) { os << "::"; }
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    // prefix_len 子网掩码位数
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
            CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return std::make_shared<IPv6Address>(baddr);
}
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
            CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return std::make_shared<IPv6Address>(baddr);
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] =
            ~CreateMask<uint8_t>(prefix_len % 8);
    for (uint32_t i = 0; i < prefix_len / 8 + 1; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return std::make_shared<IPv6Address>(subnet);
}
uint16_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

// UnixAddress
static const size_t MAX_PATH_LEN =
        sizeof(((sockaddr_un *) nullptr)->sun_path) - 1;
// 将空指针转换成sockaddr_un类型。获得sun_path字段的大小，
// 由于空指针转换后不指向有效内存，因此这个大小实际上是sun_path字段的最大长度，-1表示实际长度
// sun_path 是字符数组，末尾有个\0;
UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));// 置空
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    //offsetof(sockaddr_un, sun_path)返回的值是 sun_path 相对于 sockaddr_un 结构体起始地址的偏移量，以字节为单位
}
UnixAddress::UnixAddress(const std::string &path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;
    if (!path.empty() && path[0] == '\0') { --m_length; }
    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}
const sockaddr *UnixAddress::getAddr() const { return (sockaddr *) &m_addr; }
sockaddr *UnixAddress::getAddr() { return (sockaddr *) &m_addr; }
socklen_t UnixAddress::getAddrlen() const { return m_length; }
std::ostream &UnixAddress::insert(std::ostream &os) const {
    if (m_length > offsetof(sockaddr_un, sun_path) &&
        m_addr.sun_path[0] == '\0') {
        return os << "\\0"
                  << std::string(m_addr.sun_path + 1,
                                 m_length - offsetof(sockaddr_un, sun_path) -
                                         1);
    }
    return os << m_addr.sun_path;
}
void UnixAddress::setAddrLen(uint32_t v) { m_length = v; }

//UnknownAddress
UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}
UnknownAddress::UnknownAddress(const sockaddr &addr) {}
const sockaddr *UnknownAddress::getAddr() const { return (sockaddr *) &m_addr; }
sockaddr *UnknownAddress::getAddr() { return (sockaddr *) &m_addr; }
socklen_t UnknownAddress::getAddrlen() const { return sizeof(m_addr); }
std::ostream &UnknownAddress::insert(std::ostream &os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

}// namespace ytccc