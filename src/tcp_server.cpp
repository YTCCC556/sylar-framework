//
// Created by YTCCC on 2024/1/16.
//

#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace ytccc {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Config::Lookup("tcp_server.read_timeout", (uint64_t) (60 * 1000 * 2),
                   "tcp server read timeout");

TcpServer::TcpServer(IOManager *worker, IOManager *accept_worker)
    : m_worker(worker), m_acceptWorker(accept_worker),
      m_recvTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("ytccc/1.0.0"), m_isStop(true) {}
TcpServer::~TcpServer() {
    for (auto &i: m_socks) i->close();
    m_socks.clear();
}
bool TcpServer::bind(ytccc::Address::ptr addr) {
    std::vector<Address::ptr> addrs, fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}
bool TcpServer::bind(const std::vector<Address::ptr> &addrs,
                     std::vector<Address::ptr> &fails) {
    for (auto &addr: addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock->bind(addr)) {
            SYLAR_LOG_ERROR(g_logger)
                << "bind fail errno=" << errno << " err_str=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            SYLAR_LOG_ERROR(g_logger) << "listen fail errno=" << errno
                                      << " err_str=" << strerror(errno)
                                      << " addr=(" << addr->toString() << ")";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }
    if (!fails.empty()) {
        m_socks.clear();
        return false;
    }
    for (auto &i: m_socks) {
        SYLAR_LOG_INFO(g_logger) << "server bind success:" << *i;
    }
    return true;
}
bool TcpServer::start() {
    if (!m_isStop) { return true; }
    m_isStop = false;
    for (auto &sock: m_socks) {
        m_acceptWorker->schedule([capture0 = shared_from_this(), sock] {
            capture0->startAccept(sock);
        });
    }
    return true;
}
bool TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for (auto &sock: m_socks) {
            sock->cancelALL();
            sock->close();
        }
        m_socks.clear();
    });
    return true;
}
void TcpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient:" << *client;
}
void TcpServer::startAccept(Socket::ptr sock) {
    while (!m_isStop) {
        Socket::ptr client = sock->accept();
        if (client) {
            client->setRecvTimeout(m_recvTimeout);
            // m_worker->schedule(std::bind(&TcpServer::handleClient,shared_from_this(), client));
            m_worker->schedule([capture0 = shared_from_this(), client] {
                capture0->handleClient(client);
            });
        } else {
            SYLAR_LOG_INFO(g_logger)
                << "accept errno=" << errno << " errStr=" << strerror(errno);
        }
    }
}
}// namespace ytccc