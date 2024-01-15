//
// Created by YTCCC on 2024/1/10.
//

#ifndef YTCCC_MODULE_HTTP_PARSER_H
#define YTCCC_MODULE_HTTP_PARSER_H

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include <memory>

namespace ytccc::http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    int isFinished();
    int hasError();
    size_t execute( char *data, size_t len) ;
    HttpRequest::ptr getData() { return m_data; }
    void setError(int v) { m_error = v; }

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    // 1000:invalid method
    // 1001:invalid version
    // 1002:invalid field length
    int m_error;// 如果有错误就无需将结构体返回
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    int isFinished() ;
    int hasError() ;
    size_t execute( char *data, size_t len, size_t off) ;
    HttpResponse::ptr getData() const { return m_data; }

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;// 如果有错误就无需将结构体返回
};

}// namespace ytccc::http


#endif//YTCCC_MODULE_HTTP_PARSER_H
