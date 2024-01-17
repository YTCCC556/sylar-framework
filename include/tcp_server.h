//
// Created by YTCCC on 2024/1/16.
//

#ifndef YTCCC_MODULE_TCP_SERVER_H
#define YTCCC_MODULE_TCP_SERVER_H

#include "address.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"
#include <functional>
#include <memory>

namespace ytccc {

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(IOManager *worker = ytccc::IOManager::GetThis(),
              IOManager *accept_worker = ytccc::IOManager::GetThis());
    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr> &addrs,
                      std::vector<Address::ptr> &fails);
    virtual bool start();
    virtual bool stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string &v) { m_name = v; }
    bool isStop() const { return m_isStop; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

private:
    std::vector<Socket::ptr> m_socks;
    IOManager *m_worker;
    IOManager *m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;// 用于日志输出
    bool m_isStop;     // 是否停止
};
}// namespace ytccc


#endif//YTCCC_MODULE_TCP_SERVER_H
