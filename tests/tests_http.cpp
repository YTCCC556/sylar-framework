//
// Created by YTCCC on 2024/1/10.
//
#include "http.h"
void test_req() {
    ytccc::http::HttpRequest::ptr req(new ytccc::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");
    req->dump(std::cout) << std::endl;
}

void test_resp() {
    ytccc::http::HttpResponse::ptr rsp(new ytccc::http::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("hello sylar");
    rsp->setStatus((ytccc::http::HttpStatus) 400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char **argv) {
    // test_req();
    test_resp();
    return 0;
}