//
// Created by YTCCC on 2023/10/23.
//

#include "util.h"
#include "fiber.h"
#include "log.h"
#include <cstdint>
#include <cstdio>
#include <execinfo.h>


namespace ytccc {

ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadID() { return syscall(SYS_gettid); }

uint32_t GetFiberID() { return Fiber::GetFiberID(); }

void Backtrace(std::vector<std::string> &bt, int size, int skip) {
    void **array = (void **) malloc(sizeof(void *) * size);
    /* 使用 backtrace 函数获取当前线程的函数调用栈信息，
     * 将地址信息存储在之前分配的 array 内存块中。size 参数指定要获取的堆栈帧的最大数量。
     */
    size_t s = ::backtrace(array, size);
    /* 使用 backtrace_symbols 函数将地址信息转换为可读的字符串符号。
     * 这些字符串包含了每个堆栈帧的函数名、文件名、行号等信息。
     * 返回的 strings 是一个指针数组，每个元素都是一个字符串。
     */
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
