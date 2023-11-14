//
// Created by YTCCC on 2023/11/14.
//
#include "ytccc.h"
#include <iostream>
#include <string>

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void test_assert() {
    SYLAR_LOG_INFO(g_logger) << ytccc::BacktraceToString(10, 2, "   ");
    // assert(0==1);
    std::cout << std::string(60, '=') << std::endl;
    // SYLAR_ASSERT(false);
    SYLAR_ASSERT2(0==1,"adfasdf");
}

int main(int argc, char **argv) {
    test_assert();
    return 0;
}