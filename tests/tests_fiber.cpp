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

void func1() {
    SYLAR_LOG_INFO(g_logger) << " begin";
    ytccc::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << " end1";
    ytccc::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << " end2";

}

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "main begin -1";
    {
        ytccc::Fiber::GetThis();
        SYLAR_LOG_INFO(g_logger) << "main begin";
        ytccc::Fiber::ptr fiber(new ytccc::Fiber(func1));
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    SYLAR_LOG_INFO(g_logger) << "main end 2";
}




void test_fiber2() {
    ytccc::Fiber::GetThis();
    ytccc::Fiber::ptr fiber1(new ytccc::Fiber(func1));
    ytccc::Fiber::ptr fiber2(new ytccc::Fiber(func1));
    fiber1->swapIn();
    fiber2->swapIn();

}

int main(int argc, char **argv) {
    test_fiber();

    // ytccc::Thread::SetName("main");
    // std::vector<ytccc::Thread::ptr> thrs;
    // int thread_number = 1;
    // for (int i = 0; i < thread_number; ++i) {
    //     //使用了std::make_shared来创建一个std::shared_ptr，其中包含了一个ytccc::Thread对象。这个ytccc::Thread对象的构造函数接受两个参数
    //     auto tmp = std::make_shared<ytccc::Thread>(&test_fiber, "name");
    //     thrs.push_back(tmp);
    // }
    // for (auto i: thrs) { i->join(); }
    // std::make_shared<ytccc::Thread>(&test_fiber, "name")->join();


    // test_fiber2();
    return 0;
}