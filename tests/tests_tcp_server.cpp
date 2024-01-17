//
// Created by YTCCC on 2024/1/17.
//
#include "address.h"
#include "iomanager.h"
#include "log.h"
#include "tcp_server.h"

#include <memory>

ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    auto addr = ytccc::IPAddress::LookupAny("0.0.0.0:8033");
    // auto addr2 = std::make_shared<ytccc::UnixAddress>("/tmp/unix_addr");

    // SYLAR_LOG_INFO(g_logger) << *addr << " - " << *addr2;
    std::vector<ytccc::Address::ptr> addrs, fails;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    ytccc::TcpServer::ptr tcp_server(new ytccc::TcpServer);
    while (!tcp_server->bind(addrs, fails)) { sleep(2); }
    tcp_server->start();
}

int main(int agrc, char **argv) {
    ytccc::IOManager iom(1);
    iom.schedule(run);
    return 0;
}