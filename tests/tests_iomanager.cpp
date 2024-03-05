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

void test_timer1() {
    ytccc::IOManager iom(1);
    // IOManager继承了TimerManager
    iom.addTimer(
            500, []() { SYLAR_LOG_INFO(g_logger) << "hello timer"; }, true);
}

void test_timer2() {
    // 测试取消功能
    ytccc::IOManager iom(2);
    ytccc::Timer::ptr timer = iom.addTimer(
            500,
            [&timer]() {
                static int i = 0;
                if (++i == 3) { timer->cancel(); }
                SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
            },
            true);
}

void test_timer3() {
    // 测试reset功能
    ytccc::IOManager iom(2);
    ytccc::Timer::ptr timer = iom.addTimer(
            500,
            [&timer]() {
                static int i = 0;
                if (++i == 3) { timer->reset(2000,true); }
                if (i == 5) { timer->cancel(); }
                SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
            },
            true);
}

int main(int argc, char **argv) {
    // test1();
    test_timer1();
    // test_timer2();
    // test_timer3();
    return 0;
}
