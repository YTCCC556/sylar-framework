//
// Created by YTCCC on 2023/10/20.
//

#ifndef CLIONPROJECT_LOG_H
#define CLIONPROJECT_LOG_H

#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

/*ytccc::LogEventWrap()返回值是一个指向对象的指针或引用，那么该对象是在堆上分配的，
若返回值是对象本身，意味着对象是在栈上创建的，对象的声明周期受到函数作用域的影响。
 代码块作用域，if for while创建的变量，只能在统一代码块内访问*/
#define SYLAR_LOG_LEVEL(logger, level)                                         \
    if (logger->getLevel() <= level)                                           \
    ytccc::LogEventWrap(                                                       \
            ytccc::LogEvent::ptr(new ytccc::LogEvent(                          \
                    logger, level, __FILE__, __LINE__, 0,                      \
                    ytccc::GetThreadID(), ytccc::GetFiberID(), time(0))))      \
            .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, ytccc::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, ytccc::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, ytccc::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, ytccc::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, ytccc::LogLevel::FATAL)


namespace ytccc {

    class Logger;

    // 日志级别
    class LogLevel {
    public:
        enum Level { DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };
        static const char *Tostring(LogLevel::Level level);

    private:
    };

    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                 const char *m_file, int32_t m_line, uint32_t m_elapse,
                 uint32_t m_threadID, uint32_t m_fiberID, uint64_t m_time);
        ~LogEvent();

        [[nodiscard]] const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadID() const { return m_threadID; }
        uint32_t getFiberID() const { return m_fiberID; }
        time_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }

        std::stringstream &getSS() { return m_ss; }
        const std::string &getThreadName() const { return m_threadName; }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr;// 文件名
        int32_t m_line = 0;          // 行号
        uint32_t m_elapse = 0;       //程序运行时间毫秒
        uint32_t m_threadID = 0;     // 线程号
        uint32_t m_fiberID = 0;      // 协程号
        time_t m_time;               // 时间戳
        std::stringstream m_ss;      // 内容
        std::string m_threadName;

        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };


    //LogEventWrap
    class LogEventWrap {
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };


    // 日志格式器
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(std::string pattern);
        std::string format(std::shared_ptr<Logger> logger,
                           LogLevel::Level level, LogEvent::ptr event);
        void init();

    public:
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os,
                                std::shared_ptr<Logger> logger,
                                LogLevel::Level level, LogEvent::ptr event) = 0;
        };

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    // 日志输出地
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                         LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }

    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;// 定义输出格式
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                 LogEvent::ptr event) override;

    private:
    };

    // 输出到文件的Appender
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                 LogEvent::ptr event) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    //日志器
    class Logger : public std::enable_shared_from_this<Logger> {
    public:
        typedef std::shared_ptr<Logger> ptr;


        Logger(std::string name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string &getName() const { return m_name; }

    private:
        std::string m_name;                     //日志名称
        LogLevel::Level m_level;                //日志级别
        std::list<LogAppender::ptr> m_appenders;//Appender集合
        LogFormatter::ptr m_formatter;
    };


};// namespace ytccc


#endif//CLIONPROJECT_LOG_H
