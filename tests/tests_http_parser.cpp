//
// Created by YTCCC on 2024/1/15.
//
#include "http_parser.h"
#include "log.h"

static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

const char test_request_data[] = "GET / HTTP/1.1\r\n"
                                 "Host:www.baidu.com\r\n"
                                 "Content-length:10\r\n\r\n"
                                 "1234567890";
void test_request() {
    ytccc::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    SYLAR_LOG_ERROR(g_logger)
        << "execute rt=" << s << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished() << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength();
    tmp.resize(tmp.size() - s);
    SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
    SYLAR_LOG_INFO(g_logger) << tmp;
}

int main(int argc, char **argv) {
    test_request();
    return 0;
}
