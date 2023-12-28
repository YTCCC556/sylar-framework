//
// Created by YTCCC on 2023/12/28.
//
#include "address.h"
#include "iomanager.h"
#include "log.h"
#include "socket.h"

static ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket() {
    ytccc::IPAddress::ptr addr =
            ytccc::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr) {
        SYLAR_LOG_INFO(g_logger) << "get address:" << addr->toString();
    } else {
        SYLAR_LOG_INFO(g_logger) << "get address fail";
        return;
    }
    ytccc::Socket::ptr sock = ytccc::Socket::CreateTCP(addr);
    addr->setPort(80);
    SYLAR_LOG_INFO(g_logger) << "addr=" << addr->toString();
    if (!sock->connect(addr)) {// 没连上
        SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        SYLAR_LOG_INFO(g_logger)
                << "connect " << addr->toString() << " connect";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        SYLAR_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }
    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if (rt <= 0) {
        SYLAR_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }
    buffs.resize(rt);
    SYLAR_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char **argv) {
    ytccc::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}