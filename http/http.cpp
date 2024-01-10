//
// Created by YTCCC on 2024/1/8.
//

#include "http.h"

namespace ytccc {

namespace http {
    HttpMethod StringToHttpMethod(const std::string &m) {
#define XX(num, name, string)                                                  \
    if (strcmp(#string, m.c_str()) == 0) { return HttpMethod::name; }
        HTTP_METHOD_MAP(XX)
#undef XX
        return HttpMethod::INVALID_METHOD;
    }

    HttpMethod CharsToHttpMethod(const char *m) {
#define XX(num, name, string)                                                  \
    if (strcmp(#string, m)) { return HttpMethod::name; }
        HTTP_METHOD_MAP(XX)
#undef XX
        return HttpMethod::INVALID_METHOD;
    }

    static const char *s_method_string[] = {
#define XX(num, name, string) #string,
        HTTP_METHOD_MAP(XX)
#undef XX
    };

    const char *HttpMethodToString(const HttpMethod &m) {
        auto idx = (uint32_t) m;
        if (idx >= sizeof(s_method_string) / sizeof(s_method_string[0])) {
            return "<unknown>";
        }
        return s_method_string[idx];
    }

    const char *HttpStatusToString(const HttpStatus &s) {
        switch (s) {
#define XX(code, name, msg)                                                    \
    case HttpStatus::name:                                                     \
        return #msg;
            HTTP_STATUS_MAP(XX);
#undef XX
            default:
                return "<unknown>";
        }
    }

    bool CaseInsensitiveLess::operator()(const std::string &lhs,
                                         const std::string &rhs) const {
        return strcasecmp(lhs.c_str(), rhs.c_str());
    }

    HttpRequest::HttpRequest(uint8_t version, bool close)
        : m_method(HttpMethod::GET), m_version(version), m_close(false),
          m_path("/") {}
    std::string HttpRequest::getHeader(const std::string &key,
                                       const std::string &def) const {
        auto it = m_headers.find(key);
        return it == m_headers.end() ? def : it->second;
    }
    std::string HttpRequest::getParam(const std::string &key,
                                      const std::string &def) const {
        auto it = m_params.find(key);
        return it == m_params.end() ? def : it->second;
    }
    std::string HttpRequest::getCookie(const std::string &key,
                                       const std::string &def) const {
        auto it = m_cookie.find(key);
        return it == m_cookie.end() ? def : it->second;
    }

    void HttpRequest::setHeader(const std::string &key,
                                const std::string &val) {
        m_headers[key] = val;
    }
    void HttpRequest::setParam(const std::string &key, const std::string &val) {
        m_params[key] = val;
    }
    void HttpRequest::setCookie(const std::string &key,
                                const std::string &val) {
        m_cookie[key] = val;
    }

    void HttpRequest::delHeader(const std::string &key) {
        m_headers.erase(key);
    }
    void HttpRequest::delParam(const std::string &key) { m_params.erase(key); }
    void HttpRequest::delCookie(const std::string &key) { m_cookie.erase(key); }

    bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
        auto it = m_headers.find(key);
        if (it == m_headers.end()) { return false; }
        if (val) { *val = it->second; }
        return true;
    }
    bool HttpRequest::hasParam(const std::string &key, std::string *val) {
        auto it = m_params.find(key);
        if (it == m_params.end()) return false;
        if (val) *val = it->second;
        return true;
    }
    bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
        auto it = m_cookie.find(key);
        if (it == m_cookie.end()) return false;
        if (val) *val = it->second;
        return true;
    }

    std::ostream &HttpRequest::dump(std::ostream &os) {
        // GET /uri HTTP/1.1
        // HOST: www.baidu.com

        os << HttpMethodToString(m_method) << " " << m_path
           << (m_query.empty() ? "" : "?") << m_query
           << (m_fragment.empty() ? "" : "#") << m_fragment << " HTTP/"
           << ((uint32_t) (m_version >> 4)) << "."
           << ((uint32_t) (m_version & 0x0F)) << "\r\n";
        os << "connection:" << (m_close ? "close" : "keep-alive") << "\r\n";
        for (auto &i: m_headers) {
            if (strcmp(i.first.c_str(), "connection") != 0)
                os << i.first << ":" << i.second << "\r\n";
        }
        if (!m_body.empty()) {
            os << "content-length:" << m_body.size() << "\r\n\r\n" << m_body;
        } else {
            os << "\r\n";
        }
        return os;
    }

    HttpResponse::HttpResponse(uint8_t version, bool close)
        : m_status(HttpStatus::OK), m_version(version), m_close(close) {}
    std::string HttpResponse::getHeader(const std::string &key,
                                        const std::string &def) const {
        auto it = m_headers.find(key);
        return it == m_headers.end() ? def : it->second;
    }
    void HttpResponse::setHeader(const std::string &key,
                                 const std::string &val) {
        m_headers[key] = val;
    }
    void HttpResponse::delHeader(const std::string &key) {
        m_headers.erase(key);
    }
    std::ostream &HttpResponse::dump(std::ostream &os) {
        os << "HTTP/" << ((uint32_t) (m_version >> 4)) << "."
           << ((uint32_t) (m_version & 0x0F)) << " " << (uint32_t) m_status
           << " "
           << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
           << "\r\n";

        for (auto &i: m_headers) {
            if (strcasecmp(i.first.c_str(), "connection") != 0) {
                os << i.first << ": " << i.second << "\r\n";
            }
        }
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
        if (!m_body.empty()) {
            os << "content-length:" << m_body.size() << "\r\n\r\n" << m_body;
        } else {
            os << "\r\n";
        }
        return os;
    }
}// namespace http
}// namespace ytccc
