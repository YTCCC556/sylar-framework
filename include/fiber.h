//
// Created by YTCCC on 2023/11/15.
//

#ifndef SYLAR_FRAMEWORK_FIBER_H
#define SYLAR_FRAMEWORK_FIBER_H

#include "thread.h"
#include <functional>
#include <memory>
#include <ucontext.h>

namespace ytccc {

// 继承后，不能在栈上创建对象，
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT, // 初始化状态
        HOLD, // 暂停状态
        EXEC, // 执行中状态
        TERM, // 结束状态
        READY,// 可执行状态
        EXCEPT// 异常状态
    };

private:
    // 私有构造函数
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    // 重置协程函数，并重置状态 init，TERM
    void reset(std::function<void()> cb);
    // 切换到当前协程开始执行
    void swapIn();
    // 结束执行，切换到后台，让出执行权
    void swapOut();

    uint64_t getID() const { return m_id; }

public:
    // 设置当前协程
    static void SetThis(Fiber *f);
    // 返回当前协程
    static Fiber::ptr GetThis();
    // 协程切换到后台，设置状态为Ready状态
    static void YieldToReady();
    // 协程切换到后台，设置为Hold状态
    static void YieldToHold();
    // 总协程数
    static uint64_t TotalFibers();

    static void MainFunc();

    static uint64_t GetFiberID();

private:
    uint64_t m_id = 0;         // 协程ID
    uint32_t m_stacksize = 0;  // 协程运行栈大小
    State m_state = INIT;      // 协程状态
    ucontext_t m_ctx;          // 协程上下文
    void *m_stack = nullptr;   // 协程运行栈指针
    std::function<void()> m_cb;// 协程运行函数
};

}// namespace ytccc


#endif//SYLAR_FRAMEWORK_FIBER_H
