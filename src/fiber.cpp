//
// Created by YTCCC on 2023/11/15.
//
#include "fiber.h"
#include "atomic"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include <utility>

namespace ytccc {
static std::atomic<uint64_t> s_fiber_id{0};   // 协程id
static std::atomic<uint64_t> s_fiber_count{0};// 协程数
// 线程局部变量
static thread_local Fiber *t_fiber = nullptr;// 当前线程正在运行的协程
static thread_local Fiber::ptr t_threadFiber =
        nullptr;// 当前线程的主协程，切换到此协程即为切换到主线程运行
// 协程栈大小
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
        "fiber.stack_size", 1024 * 1024, "fiber stack size");
// static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 内存分配器 重新封装malloc
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void *vp, size_t stacksize = 0) { return free(vp); }
};
using StackAllocator = MallocStackAllocator;// 为类型引入别名

/*Fiber实现*/
// 线程的主协程
Fiber::Fiber() {
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() " << this->m_id;
    m_state = EXEC;
    SetThis(this);// 把主协程设为当前协程
    if (getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "getcontext"); }
    ++s_fiber_count;
}
// 创建新协程，创建栈空间
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(std::move(cb)) {
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber(...) id=" << m_id;
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);// 根据大小分配内存
    if (getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "getcontext"); }
    m_ctx.uc_link =
            nullptr;// 该字段指定在当前上下文完成后要切换到的下一个上下文
    m_ctx.uc_stack.ss_sp = m_stack;      // 栈的起始地址
    m_ctx.uc_stack.ss_size = m_stacksize;// 栈的大小
    // makecontext 函数用于初始化一个上下文，使其执行特定的函数
    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        // 将m_ctx所指向的上下文同func绑定，之后调用set context，以及swap context时，func会自动执行。
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
}
// 协程结束后，回收内存
Fiber::~Fiber() {
    SYLAR_LOG_INFO(g_logger)
            << "Fiber::~Fiber() id=" << m_id << " total=" << s_fiber_count;
    --s_fiber_count;
    if (m_stack) {
        // 状态不是TERM、INIT、EXCEPT(结束，初始化，一场)中的一个，报错
        SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);// 回收栈
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber *our = t_fiber;
        if (our == this) { SetThis(nullptr); }
    }
}

// 重置协程函数，并重置状态 init，TERM
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);// 是否有栈空间
    // 协程状态是否为结束或者初始化或者意外，只有这三种状态的协程允许被初始化
    SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = std::move(cb);
    if (getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "get context"); }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);// 设置回调函数
    m_state = INIT;
}
// 切换到当前协程开始执行
void Fiber::swapIn() {
    // 当前正在运行的协程切换到后台
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    // 存储当前协程的上下文，恢复this的上下文
    SYLAR_LOG_INFO(g_logger)
            << "from MainFiber(id=" << Scheduler::GetMainFiber()->m_id
            << ") swap to this(id=" << this->m_id << ")";
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swap context");
    }
}
// 结束执行，切换到后台，让出执行权
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    SYLAR_LOG_INFO(g_logger)
            << "from this(id=" << this->m_id
            << ") swap to GetMainFiber(id=" << Scheduler::GetMainFiber()->m_id
            << ")";
    // SYLAR_LOG_INFO(g_logger) << "swap to t_scheduler_fiber->ctx";
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swap context");
    }
}
void Fiber::call() {
    SetThis(this);// 将this设为当前正在运行的协程
    m_state = EXEC;
    // 强行将当前协程转换成目标执行协程，恢复m_ctx中的上下文，将当前上下文存入t_threadFiber的m_ctx中
    SYLAR_LOG_INFO(g_logger) << "from t_threadFiber(id=" << t_threadFiber->m_id
                             << ") swap to this(id=" << this->m_id << ")";
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swap context");
    }
}
void Fiber::back() {
    // 跟swapout相同逻辑，但是没有判断
    SetThis(t_threadFiber.get());// 将主协程设为当前协程
    // 保存当前上下文到m_ctx中，将主协程的上下文调出
    SYLAR_LOG_INFO(g_logger) << "from this(id=" << this->m_id
                             << ") swap to t_threadFiber(id=" << t_threadFiber->m_id << ")";
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swap context");
    }
}

// 设置当前协程，设置当前正在运行的协程
void Fiber::SetThis(Fiber *f) { t_fiber = f; }
// 返回当前协程，即当前正在执行的协程
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();// 返回智能指针
    }
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;// 设置主协程
    return t_fiber->shared_from_this();
}
// 协程切换到后台，设置状态为Ready状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}
// 协程切换到后台，设置为Hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    // cur->m_state = HOLD;
    cur->swapOut();
}
// 总协程数
uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc() {
    // GetThis()的shared_from_this()方法让引用计数加1
    // SYLAR_LOG_INFO(g_logger) << "MainFunc";
    Fiber::ptr cur = GetThis();//返回当前正在执行的协程
    SYLAR_ASSERT(cur);
    try {
        SYLAR_LOG_INFO(g_logger) << "MainFunc cur->m_cb() id=" << cur->m_id;
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
    }

    auto raw_ptr = cur.get();// 手动让t_fiber的引用计数减1
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach");
}

void Fiber::CallerMainFunc() {
    // SYLAR_LOG_INFO(g_logger) << "CallerMainFunc";
    Fiber::ptr cur = GetThis();// cur指向当前正在执行的协程
    SYLAR_ASSERT(cur);
    try {
        SYLAR_LOG_INFO(g_logger)
                << "CallerMainFunc cur->m_cb() id=" << cur->m_id;
        cur->m_cb();        // 执行回调函数
        cur->m_cb = nullptr;// 清空回调函数
        cur->m_state = TERM;// 设置状态
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
    }

    auto raw_ptr = cur.get();// 手动将计数减一
    cur.reset();             // 重置为空
    raw_ptr->back();

    SYLAR_ASSERT2(false,
                  "never reach fiber_id= " + std::to_string(raw_ptr->getID()));
}

uint64_t Fiber::GetFiberID() {
    if (t_fiber) { return t_fiber->getID(); }
    return 0;
}

}// namespace ytccc
