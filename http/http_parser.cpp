//
// Created by YTCCC on 2024/1/10.
//

#include "http_parser.h"
#include "config.h"
#include "log.h"
#include "thread.h"
#include <string.h>

namespace ytccc::http {

static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static ytccc::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    ytccc::Config::Lookup("http.request.buffer_size", (uint64_t) (4 * 1024),
                          "http request buffer size");
static ytccc::ConfigVar<uint64_t>::ptr g_http_request_buffer_max_body_size =
    ytccc::Config::Lookup("http.request.buffer_size",
                          (uint64_t) (4 * 1024 * 1024),
                          "http request max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_buffer_max_body_size = 0;

namespace {
    struct _RequestSizeIniter {
        _RequestSizeIniter() {
            s_http_request_buffer_size = g_http_request_buffer_size->getValue();
            s_http_request_buffer_max_body_size =
                g_http_request_buffer_max_body_size->getValue();

            g_http_request_buffer_size->addListener(
                [](const uint64_t &ov, const uint64_t &nv) {
                    s_http_request_buffer_size = nv;
                });
            g_http_request_buffer_max_body_size->addListener(
                [](const uint64_t &ov, const uint64_t &nv) {
                    s_http_request_buffer_max_body_size = nv;
                });
        }
    };
    static _RequestSizeIniter _init;
}// namespace

void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    HttpMethod m = CharsToHttpMethod(at);
    if (m == HttpMethod::INVALID_METHOD) {
        // error
        SYLAR_LOG_WARN(g_logger)
            << "invalid http request method" << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}
void on_request_uri(void *data, const char *at, size_t length) {}
void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setFragment(std::string(at, length));
}
void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setPath(std::string(at, length));
}
void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setQuery(std::string(at, length));
}
void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    uint8_t v = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        //error
        SYLAR_LOG_WARN(g_logger)
            << "invalid http request version:" << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}
void on_request_header_done(void *data, const char *at, size_t length) {
    // HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
}
void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    if (flen == 0) {
        SYLAR_LOG_WARN(g_logger) << "invalid http request field length ==0";
        parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen),
                                 std::string(value, vlen));
}
// HttpRequestParser
HttpRequestParser::HttpRequestParser() : m_error(0) {
    m_data.reset(new ytccc::http::HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}
int HttpRequestParser::isFinished() { return http_parser_finish(&m_parser); }
int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}
// 1：success
// -1: error
// >0: 已处理字节数，且data有效数据为len-v；
size_t HttpRequestParser::execute(char *data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

void on_response_reason(void *data, const char *at, size_t length) {}
void on_response_status(void *data, const char *at, size_t length) {}
void on_response_chunk_size(void *data, const char *at, size_t length) {}
void on_response_http_version(void *data, const char *at, size_t length) {}
void on_response_header_done(void *data, const char *at, size_t length) {}
void on_response_last_chunk(void *data, const char *at, size_t length) {}
void on_response_http_field(void *data, const char *field, size_t flen,
                            const char *value, size_t vlen) {}
// HttpResponseParser
HttpResponseParser::HttpResponseParser() {
    m_data.reset(new ytccc::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk_size;
    m_parser.http_version = on_response_http_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
}
int HttpResponseParser::isFinished() { return 0; }
int HttpResponseParser::hasError() { return 0; }
size_t HttpResponseParser::execute(char *data, size_t len, size_t off) {
    return 0;
}
}// namespace ytccc::http