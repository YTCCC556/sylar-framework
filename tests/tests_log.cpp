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
            new ytccc::StdoutLogAppender));//日志输出到哪里
    //类型不对，这里返回的是uint64，需要uint32，直接用std::hash<std::thread::id>{}(std::this_thread::get_id()) 返回size_t
    /*ytccc::LogEvent::ptr event(new ytccc::LogEvent(
            __FILE__, __LINE__, 0,
            std::hash<std::thread::id>{}(std::this_thread::get_id()),
            ytccc::GetFiberID(), time(0)));// 事件

    logger->log(ytccc::LogLevel::DEBUG, event);//输出日志
    logger->log(ytccc::LogLevel::ERROR, event);
    std::cout << "FIRST END " << std::endl;*/


    ytccc::FileLogAppender::ptr file_appender(
            new ytccc::FileLogAppender("./log.txt"));
    ytccc::LogFormatter::ptr fmt(new ytccc::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(ytccc::LogLevel::ERROR);

    logger->addAppender(file_appender);

    std::cout << "hello log" << std::endl;
    // todo 有时候以下测试会失效，原因未知，不输出任何内容
    //以流的形式打印日志 而不是SYLAR_LOG_INFO(logger，"test macro");
    SYLAR_LOG_INFO(logger)
            << "test macro";// LogEventWrap在这行代码的作用域内创建并摧毁
    SYLAR_LOG_DEBUG(logger) << "test macro debug";
    SYLAR_LOG_ERROR(logger) << "test macro error";
    SYLAR_LOG_FATAL(logger) << "test macro fatal";
    SYLAR_LOG_WARN(logger) << "test macro warn";
    std::cout << "end log" << std::endl;

    SYLAR_LOG_FMT_INFO(logger, "test macro fmt info %s", "aa");
    SYLAR_LOG_FMT_DEBUG(logger, "test macro fmt debug %s", "aa");
    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    SYLAR_LOG_FMT_FATAL(logger, "test macro fmt fatal %s", "aa");
    SYLAR_LOG_FMT_WARN(logger, "test macro fmt warn %s", "aa");
    std::cout << "fmt log" << std::endl;

    // 不用每次去创建log，使用LoggerManager管理log，用的时候直接在内部拿就可以了。
    auto l = ytccc::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(logger) << "xxx";
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT());
    return 0;
}
