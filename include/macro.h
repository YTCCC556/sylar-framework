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
                    << "ASSERTION: " #x << "\nbacktrace:\n"                     \
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

#endif//SYLAR_FRAMEWORK_MACRO_H
