//
// Created by YTCCC on 2023/12/27.
//
#include "socket.h"
#include "fd_manager.h"
#include "hook.h"
#include "log.h"
#include "macro.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace ytccc {
ytccc::Logger::ptr socket_logger = SYLAR_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(ytccc::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}
Socket::ptr Socket::CreateUDP(ytccc::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    return sock;
}
Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}
Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    return sock;
}
Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    return sock;
}
Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}
Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1), m_family(family), m_type(type), m_protocol(protocol),
      m_isConnected(false) {}
Socket::~Socket() { close(); }

int64_t Socket::getSendTimeout() const {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) return (int64_t) ctx->getTimeout(SO_SNDTIMEO);
    return -1;
}
bool Socket::setSendTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    return setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() const {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) return (int64_t) ctx->getTimeout(SO_RCVTIMEO);
    return -1;
}
bool Socket::setRecvTimeout(int64_t v) {
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    return setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void *result,
                       socklen_t *len) const {
    if (getsockopt(m_sock, level, option, result, (socklen_t *) len)) {
        SYLAR_LOG_DEBUG(socket_logger)
            << "getOption sock=" << m_sock << " level=" << level
            << " option=" << option << " errno=" << errno
            << " err_str=" << strerror(errno);
        return false;
    }
    return true;
}
bool Socket::setOption(int level, int option, const void *result,
                       socklen_t len) const {
    // setsockopt 用于设置套接字选项，可以影响套接字的行为，通常在套接字创建后，绑定之前或则和连接之前使用。
    if (setsockopt(m_sock, level, option, result, (socklen_t) len)) {
        SYLAR_LOG_DEBUG(socket_logger)
            << "getOption sock=" << m_sock << " level=" << level
            << " option=" << option << " error=" << errno
            << " err_str=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept() const {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if (newsock == -1) {
        SYLAR_LOG_ERROR(socket_logger)
            << "accept(" << m_sock << ") errno=" << errno
            << " err_str=" << strerror(errno);
        return nullptr;
    }
    if (sock->init(newsock)) { return sock; }
    SYLAR_LOG_INFO(socket_logger) << "init error newsock=" << newsock;
    return nullptr;
}

bool Socket::bind(const Address::ptr &addr) {
    if (SYLAR_UNLICKLY(!isVaild())) {// 不等于有效
        newSock();
        if (SYLAR_UNLICKLY(!isVaild())) { return false; }
    }
    if (SYLAR_UNLICKLY(addr->getFamily() !=
                       m_family)) {// sock类型和addr类型不同
        SYLAR_LOG_ERROR(socket_logger)
            << "bind sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
    }
    if (::bind(m_sock, addr->getAddr(), addr->getAddrlen())) {
        SYLAR_LOG_ERROR(socket_logger)
            << "bind errno=" << errno << " str_err=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}
bool Socket::connect(const Address::ptr &addr, uint64_t timeout_ms) {
    // 封装原始connect函数
    m_RemoteAddress = addr;
    if (SYLAR_UNLICKLY(!isVaild())) {// 不等于有效
        newSock();
        if (SYLAR_UNLICKLY(!isVaild())) { return false; }
    }
    if (SYLAR_UNLICKLY(addr->getFamily() !=
                       m_family)) {// sock类型和addr类型不同
        SYLAR_LOG_ERROR(socket_logger)
            << "connect sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
    }
    if (timeout_ms == (uint64_t) -1) {
        if (::connect(m_sock, addr->getAddr(), addr->getAddrlen())) {
            SYLAR_LOG_ERROR(socket_logger)
                << "sock=" << m_sock << " connect(" << addr->toString()
                << ") error=" << errno << " err_str=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(),
                                   timeout_ms)) {
            SYLAR_LOG_ERROR(socket_logger)
                << "sock=" << m_sock << " connect(" << addr->toString()
                << ") timeout_ms= " << timeout_ms << "error=" << errno
                << "err_str" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}
bool Socket::listen(int backlog) const {
    if (!isVaild()) {
        SYLAR_LOG_ERROR(socket_logger) << "listen error sock=" << m_sock;
        return false;
    }
    if (::listen(m_sock, backlog)) {
        SYLAR_LOG_ERROR(socket_logger)
            << "listen error errno=" << errno << " err_str=" << strerror(errno);
        return false;
    }
    return true;
}
bool Socket::close() {
    if (!m_isConnected && m_sock == -1) { return true; }
    m_isConnected = false;
    if (m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

int Socket::send(const void *buffer, size_t length, int flags) const {
    if (isConnected()) { return (int) ::send(m_sock, buffer, length, flags); }
    return -1;
}
int Socket::send(const iovec *buffers, size_t length, int flags) const {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *) buffers;
        msg.msg_iovlen = length;
        return (int) ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}
int Socket::sendTo(const void *buffer, size_t length, const Address::ptr &to,
                   int flags) const {
    if (isConnected()) {
        return (int) ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                              to->getAddrlen());
    }
    return -1;
}
int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr &to,
                   int flags) const {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *) buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrlen();
        return (int) ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) const {
    if (isConnected()) { return (int) ::recv(m_sock, buffer, length, flags); }
    return -1;
}
int Socket::recv(iovec *buffers, size_t length, int flags) const {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *) buffers;
        msg.msg_iovlen = length;
        return (int) ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}
int Socket::recvFrom(void *buffer, size_t length, const Address::ptr &from,
                     int flags) const {
    if (isConnected()) {
        socklen_t len = from->getAddrlen();
        return (int) ::recvfrom(m_sock, buffer, length, flags, from->getAddr(),
                                &len);
    }
    return -1;
}
int Socket::recvFrom(iovec *buffers, size_t length, const Address::ptr &from,
                     int flags) const {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *) buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrlen();
        return (int) ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if (m_RemoteAddress) { return m_RemoteAddress; }
    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrlen();
    if (getpeername(m_sock, result->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(socket_logger)
            << "getpeername error sock=" << m_sock << " errno=" << errno
            << " str_err=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_RemoteAddress = result;
    return m_RemoteAddress;
}
Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) { return m_localAddress; }
    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrlen();
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(socket_logger)
            << "getsockname error sock=" << m_sock << " errno=" << errno
            << " str_err=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

bool Socket::isVaild() const { return m_sock != -1; }
int Socket::getError() const {
    int error = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) { return -1; }
    return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
    os << "[socket sock=" << m_sock << " is_connected=" << m_isConnected
       << " family=" << m_family << " type=" << m_type
       << " protocol=" << m_protocol;
    if (m_localAddress) os << " localAddress=" << m_localAddress->toString();
    if (m_RemoteAddress) os << " remoteAddress=" << m_RemoteAddress->toString();
    os << ")";
    return os;
}

bool Socket::cancelRead() const {
    return IOManager::GetThis()->cancelEvent(m_sock, ytccc::IOManager::READ);
}
bool Socket::cancelWrite() const {
    return IOManager::GetThis()->cancelEvent(m_sock, ytccc::IOManager::WRITE);
}
bool Socket::cancelAccept() const {
    return IOManager::GetThis()->cancelEvent(m_sock, ytccc::IOManager::READ);
}
bool Socket::cancelALL() const {
    return IOManager::GetThis()->cancelAll(m_sock);
}

bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if (ctx && ctx->isSocked() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (m_type == SOCK_STREAM) { setOption(IPPROTO_TCP, TCP_NODELAY, val); }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if (SYLAR_LICKLY(m_sock != -1)) {
        initSock();
    } else {
        SYLAR_LOG_ERROR(socket_logger)
            << "socket(" << m_family << " , " << m_type << " , " << m_protocol
            << ") errno=" << errno << " err_str" << strerror(errno);
    }
}

std::ostream &operator<<(std::ostream &os, const Socket &addr) {
    return addr.dump(os);
}

}// namespace ytccc