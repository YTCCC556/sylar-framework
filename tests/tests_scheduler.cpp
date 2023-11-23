//
// Created by YTCCC on 2023/11/21.
//
#include "ytccc.h"

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void test_fiber1() { SYLAR_LOG_INFO(g_logger) << "test in fiber"; }

void test_fiber2() {
    static int s_count = 5;
    SYLAR_LOG_INFO(g_logger)
            << "-----------------------------[test in fiber s_count=" << s_count
            << "]";

    // sleep(1);
    if (--s_count >= 0) {
        // ytccc::Scheduler::GetThis()->schedule(&test_fiber2); // 不指定线程执行
        ytccc::Scheduler::GetThis()->schedule(&test_fiber2,
                                              ytccc::GetThreadID()); // 指定线程执行
    }
}

int main(int argc, char **argv) {
    ytccc::Thread::SetName("main");
    ytccc::Scheduler sc(3, false, "test");
    // ytccc::Scheduler sc;
    SYLAR_LOG_INFO(g_logger) << "-------------start--------------";
    sc.start();
    SYLAR_LOG_INFO(g_logger) << "-------------test_fiber--------------";
    sc.schedule(&test_fiber2);
    SYLAR_LOG_INFO(g_logger) << "-------------end--------------";
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "-------------over---------------";
    return 0;
}