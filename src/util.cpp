//
// Created by YTCCC on 2023/10/23.
//

#include "util.h"
#include "log.h"
#include <cstdint>
#include <cstdio>
#include <execinfo.h>


namespace ytccc {

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadID() { return syscall(SYS_gettid); }

uint32_t GetFiberID() { return 0; }

void Backtrace(std::vector<std::string> &bt, int size, int skip) {
    void **array = (void **) malloc(sizeof(void *) * size);
    size_t s = ::backtrace(array, size);

    char **strings = backtrace_symbols(array, s);
    if (strings == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
    }

    for (size_t i = skip; i < s; ++i) { bt.emplace_back(strings[i]); }
    free(strings);
}

std::string BacktraceToString(int size, int skip, const std::string &prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


}// namespace ytccc
