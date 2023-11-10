//
// Created by YTCCC on 2023/11/9.
//
#include "log.h"
#include "thread.h"
#include "config.h"

ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();
ytccc::RWMutex s_mutex;

void fun1() {
    SYLAR_LOG_INFO(g_logger)
            << "name: " << ytccc::Thread::GetName() << " this.name "
            << ytccc::Thread::GetThis()->getName()
            << " id:" << ytccc::GetThreadID() << " this.id "
            << ytccc::Thread::GetThis()->getId();
}

void test_thread() {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<ytccc::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        ytccc::Thread::ptr thr(
                new ytccc::Thread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for (int i = 0; i < 5; ++i) { thrs[i]->join(); }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    // return 0;
}

volatile int count = 0;
void fun2() {
    SYLAR_LOG_INFO(g_logger)
            << "name: " << ytccc::Thread::GetName() << " this.name "
            << ytccc::Thread::GetThis()->getName()
            << " id:" << ytccc::GetThreadID() << " this.id "
            << ytccc::Thread::GetThis()->getId();
    for (int i = 0; i < 10000; ++i) {
        ytccc::RWMutex::WriteLock lock(s_mutex);
        // ytccc::RWMutex::ReadLock lock(s_mutex);
        ++count;
    }
}

void test_thread2() {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<ytccc::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        ytccc::Thread::ptr thr(
                new ytccc::Thread(&fun2, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for (int i = 0; i < 5; ++i) { thrs[i]->join(); }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count:" << count;
    // return 0;
}

void fun3() {
    while (true) SYLAR_LOG_INFO(g_logger) << std::string(2, 'x');
}

void fun4() {
    while (true) SYLAR_LOG_INFO(g_logger) << std::string(2, '=');
}

void test_thread3() {
    SYLAR_LOG_INFO(g_logger) << "thread test 3 begin";
    YAML::Node root = YAML::LoadFile("../bin/conf/log2.yml");
    ytccc::Config::LoadFromYaml(root);
    std::vector<ytccc::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i) {
        ytccc::Thread::ptr thr(
                new ytccc::Thread(&fun3, "name_" + std::to_string(i * 2)));
        ytccc::Thread::ptr thr2(
                new ytccc::Thread(&fun4, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    for (auto & thr : thrs) {
        thr->join();
    }

}

#include <iostream>
#include <fstream>

int test_open() {
    // 打开文件
    std::ofstream outputFile("example.txt");

    // 检查文件是否成功打开
    if (!outputFile.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    // 向文件中写入内容
    outputFile << "Hello, this is some content in the file." << std::endl;
    outputFile << "You can add more lines if needed." << std::endl;

    // 关闭文件
    outputFile.close();

    std::cout << "File created and content written successfully." << std::endl;

    return 0;
}

int main(int argc, char **argv) {
    // test_thread();
    // test_thread2();
    test_thread3();
    // test_open();
}
