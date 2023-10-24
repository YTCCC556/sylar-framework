//
// Created by YTCCC on 2023/10/23.
//

#include "util.h"
#include <cstdint>
#include <cstdio>

namespace ytccc{

    pid_t GetThreadID(){
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberID(){
        return 0;
    }

}
