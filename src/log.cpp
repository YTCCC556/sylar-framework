//
// Created by YTCCC on 2023/10/20.
//

#include "../include/log.h"
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <stdarg.h>


namespace ytccc {

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        explicit MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getContent();
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << LogLevel::Tostring(level);
        }
    };
    class FilenameFormatItem : public LogFormatter::FormatItem {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getFile();
        }
    };
    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getLine();
        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << logger->getName();
        }
    };
    class ThreadIDFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIDFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getThreadID();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class FiberIDFormatItem : public LogFormatter::FormatItem {
    public:
        FiberIDFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getFiberID();
        }
    };
    class DateTimeFormatItem : public LogFormatter::FormatItem {
    public:
        //        DateTimeFormatItem(const std::string& str = "") {}
        DateTimeFormatItem(const std::string &format = "%Y:%m:%d %H:%M:%S")
            : m_format(format) {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string &str) : m_string(str) {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                    LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    //LogEvent实现
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       const char *m_file, int32_t m_line, uint32_t m_elapse,
                       uint32_t m_threadID, uint32_t m_fiberID, uint64_t m_time)
        : m_logger(std::move(logger)), m_level(level), m_file(m_file),
          m_line(m_line), m_elapse(m_elapse), m_threadID(m_threadID),
          m_fiberID(m_fiberID), m_time(m_time) {}
    LogEvent::~LogEvent() {}


    void LogEvent::format(const char *fmt, ...){
        va_list al;
        va_start(al,fmt);
        format(fmt,al);
        va_end(al);
    }
    void LogEvent::format(const char *fmt, va_list al){
        char* buf = nullptr;
        //指针的取地址运算符 & 获取的是指向变量或对象的地址
        int len = vasprintf(&buf,fmt,al);
        if(len!=-1){
            m_ss << std::string(buf,len);
            free(buf);
        }
    }

    //LogEventWrap
    LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {
        //        wrap析构的时候将event写入
    }
    LogEventWrap::~LogEventWrap() {
        //获得指向logger对象的指针，并调用log方法
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }
    std::stringstream &LogEventWrap::getSS() {
        return m_event->getSS();
    }


    //LogLevel实现
    const char *LogLevel::Tostring(LogLevel::Level level) {
        switch (level) {
#define XX(name)                                                               \
    case LogLevel::name:                                                       \
        return #name;                                                          \
        break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
            default:
                return "UNKNOW";
        }
        return "UBKNOW";
    }


    //LogFormatter
    LogFormatter::LogFormatter(std::string pattern)
        : m_pattern(std::move(pattern)) {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                     LogLevel::Level level,
                                     LogEvent::ptr event) {
        std::stringstream ss;
        for (auto &i: m_items) { i->format(ss, logger, level, event); }
        return ss.str();
    }
    //%xxx %xxx(xxx) %%转义 三种格式
    void LogFormatter::init() {
        // str,format,type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string n_str;
        for (size_t i = 0; i < m_pattern.size(); ++i) {
            if (m_pattern[i] != '%') {
                n_str.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size()) {
                if (m_pattern[i + 1] == '%') {
                    n_str.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size()) {
                if (!fmt_status &&
                    (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                     m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if (fmt_status == 0) {
                    if (m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1;//解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                } else if (fmt_status == 1) {
                    if (m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1,
                                               n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if (n == m_pattern.size()) {
                    if (str.empty()) { str = m_pattern.substr(i + 1); }
                }
            }
            if (fmt_status == 0) {
                if (!n_str.empty()) {
                    //vec.push_back(std::make_tuple(n_str, "", 0));
                    vec.emplace_back(n_str, std::string(), 0);
                    n_str.clear();
                }

                vec.emplace_back(str, fmt, 1);
                i = n - 1;
            } else if (fmt_status == 1) {
                std::cout << "pattern parse error: " << m_pattern << " - "
                          << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.emplace_back("<<pattern_error>>", fmt, 0);
            }
        }
        if (!n_str.empty()) { vec.emplace_back(n_str, "", 0); }

        static std::map<std::string,
                        std::function<LogFormatter::FormatItem::ptr(
                                const std::string &str)>>
                s_format_items = {
#define XX(str, C)                                                             \
    {                                                                          \
        #str, [](const std::string &fmt) {                                     \
            return LogFormatter::FormatItem::ptr(new C(fmt));                  \
        }                                                                      \
    }

                        XX(m, MessageFormatItem),   //m:消息
                        XX(p, LevelFormatItem),     //p:日志级别
                        XX(r, ElapseFormatItem),    //r:累计毫秒数
                        XX(c, NameFormatItem),      //c:日志名称
                        XX(t, ThreadIDFormatItem),  //t:线程id
                        XX(n, NewLineFormatItem),   //n:换行
                        XX(d, DateTimeFormatItem),  //d:时间
                        XX(f, FilenameFormatItem),  //f:文件名
                        XX(l, LineFormatItem),      //l:行号
                        XX(T, TabFormatItem),       //T:Tab
                        XX(F, FiberIDFormatItem),   //F:协程id
                        XX(N, ThreadNameFormatItem),//N:线程名称

#undef XX
                };
        //std::vector<std::tuple<std::string, std::string, int>> vec;
        for (auto &i: vec) {
            if (std::get<2>(i) == 0) {
                // string类型
                m_items.push_back(
                        FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            } else {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end()) {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem(
                            "<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                } else {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            //std::cout << std::get<0>(i) << "-" << std::get<1>(i) << "-" << std::get<2>(i) << std::endl;
        }
    }


    //LogAppender
    void LogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {}
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                                LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            std::string str = m_formatter->format(logger, level, event);
            std::cout << str << std::endl;
        }
    }
    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename) {}

    void FileLogAppender::log(std::shared_ptr<Logger> logger,
                              LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }
    bool FileLogAppender::reopen() {
        if (m_filestream) { m_filestream.close(); }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    //Logger
    //Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG) {
    Logger::Logger(const std::string name)
        : m_name(name), m_level(LogLevel::DEBUG) {
        m_formatter.reset(new LogFormatter(
                "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self =
                    shared_from_this();//std::shared_ptr 无法通过this指针创建，会导致循环引用，
            for (auto &i: m_appenders) { i->log(self, level, event); }
        }
    }

    void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }
    void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }
    void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }
    void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }
    void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

    void Logger::addAppender(LogAppender::ptr appender) {
        //保证appender要有格式
        if (!appender->getFormatter()) { appender->setFormatter(m_formatter); }
        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            m_appenders.erase(it);
            break;
        }
    }


    static std::string name = "ytccc";

}// namespace ytccc
