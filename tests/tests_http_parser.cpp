//
// Created by YTCCC on 2024/1/15.
//
#include "http_parser.h"
#include "log.h"

static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

const char test_request_data[] = "GET / HTTP/1.1\r\n"
                                 "Host:www.baidu.com\r\n"
                                 "Content-length: 10\r\n\r\n"
                                 "1234567890";
void test_request() {
    ytccc::http::HttpRequestParser parser;
    ytccc::http::HttpRequestParser::ptr parser_ptr(
        new ytccc::http::HttpRequestParser());
    std::string tmp1 = test_request_data;
    std::string tmp2 = test_request_data;
    size_t s = parser.execute(&tmp1[0], tmp1.size());
    size_t s2 = parser_ptr->execute(&tmp2[0], tmp2.size());
    SYLAR_LOG_INFO(g_logger)
        << "execute rt=" << s << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished() << " total=" << tmp1.size()
        << " content_length=" << parser.getContentLength();
    SYLAR_LOG_INFO(g_logger)
        << "execute rt=" << s << " has_error=" << parser_ptr->hasError()
        << " is_finished=" << parser_ptr->isFinished() << " total=" << tmp1.size()
        << " content_length=" << parser_ptr->getContentLength();
    tmp1.resize(tmp1.size() - s);
    SYLAR_LOG_INFO(g_logger) << "\n" << parser.getData()->toString();
    SYLAR_LOG_INFO(g_logger) << "\n" << tmp1;
}

const char test_response_data[] =
    "HTTP/1.1 200 OK\r\n"
    "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
    "Server: Apache\r\n"
    "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
    "ETag: \"51-47cf7e6ee8400\"\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: 81\r\n"
    "Cache-Control: max-age=86400\r\n"
    "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
    "Connection: Close\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html>\r\n"
    "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
    "</html>\r\n";

void test_response() {
    ytccc::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    SYLAR_LOG_INFO(g_logger)
        << "execute rt=" << s << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished() << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength()
        << " tem[s]=" << tmp[s];
    tmp.resize(tmp.size() - s);
    SYLAR_LOG_INFO(g_logger) << "\n" << parser.getData()->toString();
    SYLAR_LOG_INFO(g_logger) << "\n" << tmp;
}

int main(int argc, char **argv) {
    test_request();
    SYLAR_LOG_INFO(g_logger) << "-----------------------------------";
    test_response();
    return 0;
}
