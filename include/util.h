//
// Created by YTCCC on 2023/10/23.
//

#ifndef SYLAR_FRAMEWORK_UTIL_H
#define SYLAR_FRAMEWORK_UTIL_H

#include <cstdio>
#include <stdint.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <vector>
#include <zconf.h>


namespace ytccc {

pid_t GetThreadID();
uint32_t GetFiberID();

void Backtrace(std::vector<std::string> &bt, int size, int skip);
// 返回当前栈的内容
std::string BacktraceToString(int size, int skip = 1,
                              const std::string &prefix = "");

std::string GetRelative(std::string path);
}// namespace ytccc


#endif//SYLAR_FRAMEWORK_UTIL_H
