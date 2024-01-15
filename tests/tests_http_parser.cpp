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
void test() {
    ytccc::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    SYLAR_LOG_INFO(g_logger)
        << "execute rt=" << s << "has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished();
    SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
}

int main(int argc, char **argv) {
    test();
    return 0;
}
