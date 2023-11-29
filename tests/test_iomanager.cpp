//
// Created by YTCCC on 2023/11/28.
//

#include "ytccc.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
int sock = 0;
void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    // setsockopt();
    fcntl(sock, F_SETFL, O_NONBLOCK);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *) &addr, sizeof(addr))) {

    } else if (errno == EINPROGRESS) {
        SYLAR_LOG_INFO(g_logger)
                << "add event errno=" << errno << " " << strerror(errno);
        ytccc::IOManager::GetThis()->addEvent(
                sock, ytccc::IOManager::READ,
                []() { SYLAR_LOG_INFO(g_logger) << "read callback"; });
        ytccc::IOManager::GetThis()->addEvent(
                sock, ytccc::IOManager::WRITE, []() {
                    SYLAR_LOG_INFO(g_logger) << "write callback";
                    ytccc::IOManager::GetThis()->cancelEvent(
                            sock, ytccc::IOManager::READ);
                    close(sock);
                });
    } else {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    ytccc::IOManager iom(1, false, "test");
    iom.schedule(&test_fiber);
}

int main(int argc, char **argv) {
    test1();
    return 0;
}
