//
// Created by YTCCC on 2023/10/23.
//

#ifndef SYLAR_FRAMEWORK_UTIL_H
#define SYLAR_FRAMEWORK_UTIL_H

#include <cstdio>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <zconf.h>

namespace ytccc {

pid_t GetThreadID();
uint32_t GetFiberID();

}// namespace ytccc


#endif//SYLAR_FRAMEWORK_UTIL_H
