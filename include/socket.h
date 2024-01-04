//
// Created by YTCCC on 2023/12/27.
//

#ifndef YTCCC_MODULE_SOCKET_H
#define YTCCC_MODULE_SOCKET_H

#include "address.h"
#include "noncopyable.h"
#include <memory>

namespace ytccc {
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;
    enum Type { TCP = SOCK_STREAM, UDP = SOCK_DGRAM };
    enum Family { IPv4 = AF_INET, IPv6 = AF_INET6, UNIX = AF_UNIX };

    static Socket::ptr CreateTCP(ytccc::Address::ptr address);
    static Socket::ptr CreateUDP(ytccc::Address::ptr address);
    static Socket::ptr CreateTCPSocket();// 创建ipv4 tcp socket
    static Socket::ptr CreateUDPSocket();
    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();
    static Socket::ptr CreateUnixTCPSocket();// unix tcp socket
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    int64_t getSendTimeout() const;
    bool setSendTimeout(int64_t v);

    int64_t getRecvTimeout() const;
    bool setRecvTimeout(int64_t v);

    /**
    * @brief 获取sockopt模板 @see getsockopt
    */
    bool getOption(int level, int option, void *result, socklen_t *len) const;
    template<class T>
    bool getOption(int level, int option, T &result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }
    bool setOption(int level, int option, const void *result,
                   socklen_t len) const;
    template<class T>
    bool setOption(int level, int option, const T &value) {
        return setOption(level, option, &value, sizeof(T));
    }

    Socket::ptr accept() const;// 接收connect连接

    bool bind(const Address::ptr &addr);// 绑定地址
    bool connect(const Address::ptr &addr, uint64_t timeout_ms = -1);// 连接地址
    bool listen(int backlog = SOMAXCONN) const;// 监听地址
    bool close();                              // 关闭连接

    int send(const void *buffer, size_t length, int flags = 0) const;// 发送数据
    int send(const iovec *buffers, size_t length, int flags = 0) const;
    int sendTo(const void *buffer, size_t length, const Address::ptr &to,
               int flags = 0) const;
    int sendTo(const iovec *buffers, size_t length, const Address::ptr &to,
               int flags = 0) const;

    int recv(void *buffer, size_t length, int flags = 0) const;// 接受数据
    int recv(iovec *buffers, size_t length, int flags = 0) const;
    int recvFrom(void *buffer, size_t length, const Address::ptr &from,
                 int flags = 0) const;
    int recvFrom(iovec *buffers, size_t length, const Address::ptr &from,
                 int flags = 0) const;

    Address::ptr getRemoteAddress(); // 获取远端地址
    Address::ptr getLocalAddress();// 获取本地地址

    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }

    bool isConnected() const { return m_isConnected; }
    bool isVaild() const;
    int getError() const;

    std::ostream &dump(std::ostream &os) const;
    int getSocket() const { return m_sock; }

    bool cancelRead() const;
    bool cancelWrite() const;
    bool cancelAccept() const;
    bool cancelALL() const;

private:
    bool init(int sock);
    void initSock();
    void newSock();

private:
    int m_sock; // sock句柄
    int m_family; // 协议簇
    int m_type; // 类型
    int m_protocol; // 协议
    bool m_isConnected; // 是否连接

    Address::ptr m_localAddress; // 本地地址
    Address::ptr m_RemoteAddress; // 远端地址
};
}// namespace ytccc


#endif//YTCCC_MODULE_SOCKET_H
