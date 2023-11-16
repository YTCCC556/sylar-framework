//
// Created by YTCCC on 2023/11/15.
//
#include <memory>

#include "ytccc.h"

ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
    ytccc::Fiber::YieldToHold();
    // ytccc::Fiber::GetThis()->swapOut();
    SYLAR_LOG_INFO(g_logger) << "run in fiber end";
    ytccc::Fiber::YieldToHold();
}

void test_fiber() {

    SYLAR_LOG_INFO(g_logger) << "main begin -1";
    {
        ytccc::Fiber::GetThis();
        SYLAR_LOG_INFO(g_logger) << "main begin";
        ytccc::Fiber::ptr fiber(new ytccc::Fiber(run_in_fiber));
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    SYLAR_LOG_INFO(g_logger) << "main end 2";
}

int main(int argc, char **argv) {
    ytccc::Thread::SetName("main");
    std::vector<ytccc::Thread::ptr> thrs;
    // test_fiber();
    for (int i = 0; i < 3; ++i) {
        thrs.push_back(std::make_shared<ytccc::Thread>(
                &test_fiber, "name_" + std::to_string(i)));
    }
    for (auto i: thrs) { i->join(); }
    return 0;
}