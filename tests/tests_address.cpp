//
// Created by YTCCC on 2023/12/25.
//
#include "address.h"
#include "log.h"

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void test() {
    std::vector<ytccc::Address::ptr> addrs;
    bool v = ytccc::Address::Lookup(addrs, "www.baidu.com");
    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }
    for (size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}
void test_iface() {
    std::multimap<std::string, std::pair<ytccc::Address::ptr, uint32_t>>
            results;
    bool v = ytccc::Address::GetInterfaceAddress(results);
    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return;
    }
    for (auto &i: results) {
        SYLAR_LOG_INFO(g_logger)
                << i.first << " - " << i.second.first->toString() << " - "
                << i.second.second;
    }
}
void test_ipv4() {
    auto addr = ytccc::IPAddress::Create("www.baidu.com");
    if (addr) { SYLAR_LOG_INFO(g_logger) << addr->toString(); }
}
int main(int argc, char **argv) {
    test();
    // test_iface();
    // test_ipv4();
    return 0;
}