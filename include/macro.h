//
// Created by YTCCC on 2023/11/14.
//

#ifndef SYLAR_FRAMEWORK_MACRO_H
#define SYLAR_FRAMEWORK_MACRO_H

#include "util.h"
#include <cassert>
#include <string>

#define SYLAR_ASSERT(x)                                                        \
    {                                                                          \
        if (!(x)) {                                                            \
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                  \
                    << "ASSERTION: " #x << "\nbacktrace:\n"                    \
                    << ytccc::BacktraceToString(100, 2, "    ");               \
            assert(x);                                                         \
        }                                                                      \
    }

#define SYLAR_ASSERT2(x, w)                                                    \
    {                                                                          \
        if (!(x)) {                                                            \
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                  \
                    << "ASSERTION:" #x << "\n"                                 \
                    << w << "\nbacktrace:\n"                                   \
                    << ytccc::BacktraceToString(100, 2, "    ");               \
            assert(x);                                                         \
        }                                                                      \
    }

#if defined __GUNC__ || defined __llvm__
#define SYLAR_LICKLY(x) __buildtin_expect(!!(x), 1)
#define SYLAR_UNLICKLY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LICKLY(x) (x)
#define SYLAR_UNLICKLY(x) (x)
#endif

#endif//SYLAR_FRAMEWORK_MACRO_H
