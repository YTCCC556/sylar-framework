//
// Created by YTCCC on 2023/10/20.
//
#include "log.h"
#include "util.h"
#include <iostream>
#include <thread>

int main(int argc, char **grav) {
    std::cout << "BEGIN RUN " << std::endl;
    ytccc::Logger::ptr logger(new ytccc::Logger);//日志器
    logger->addAppender(ytccc::LogAppender::ptr(
            new ytccc::StdoutLogAppender));//日志输出到哪里 `
    //类型不对，这里返回的是uint64，需要uint32，直接用std::hash<std::thread::id>{}(std::this_thread::get_id()) 返回size_t
    /*ytccc::LogEvent::ptr event(new ytccc::LogEvent(
            __FILE__, __LINE__, 0,
            std::hash<std::thread::id>{}(std::this_thread::get_id()),
            ytccc::GetFiberID(), time(0)));// 事件

    logger->log(ytccc::LogLevel::DEBUG, event);//输出日志
    logger->log(ytccc::LogLevel::ERROR, event);
    std::cout << "FIRST END " << std::endl;*/

    SYLAR_LOG_INFO(logger) << "test macro";


    return 0;
}
