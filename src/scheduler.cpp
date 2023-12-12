//
// Created by YTCCC on 2023/11/20.
//

#include "scheduler.h"

#include <utility>
namespace ytccc {
static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_scheduler_fiber = nullptr;
// scheduler 实现
Scheduler::Scheduler(size_t threads, bool use_caller, std::string name)
    : m_name(std::move(name)) {
    SYLAR_ASSERT(threads > 0);
    if (use_caller) {
        ytccc::Fiber::GetThis();// 初始化主协程
        --threads;
        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;// TODO ?当前正在运行的协程
        // 新建协程绑定cb并赋予调度协程，
        // 会在setcontext或swapcontext中调用makecontext绑定的func，在func中调用cb，也就是run()
        m_rootFiber.reset(new Fiber([this] { run(); }, 0, true));
        ytccc::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get();// 调度协程
        m_rootThread = ytccc::GetThreadID();  // 调度器线程ID
        m_threadIDs.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}// use_caller 是否使用当前线程执行任务

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
        SYLAR_LOG_DEBUG(g_logger) << "Scheduler::~Scheduler() " << this->m_name;
    }
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }
Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

void Scheduler::start() {
    {// 线程池，启动线程
        MutexType::Lock lock(m_mutex);
        if (!m_stopping) {
            return;// 默认为true，若为false，表示启动，返回
        }
        m_stopping = false;// 启动，切换标志位
        SYLAR_ASSERT(m_threads.empty());
        // 根据线程数量向线程池中添加线程
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread([this] { run(); },
                                          m_name + "_" + std::to_string(i)));
            m_threadIDs.push_back(m_threads[i]->getId());
        }
        lock.unlock();
    }
    // TODO syalr中被注释
    // if (m_rootFiber) {// 调度协程，use_caller=true 有效
    //     SYLAR_LOG_INFO(g_logger) << "m_rootFiber.call() " << m_rootFiber << " "
    //                              << m_rootFiber->getState();
    //     m_rootFiber->call();// 调度协程调用call方法
    //     // SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}
void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 &&
        (m_rootFiber->getState() == Fiber::TERM ||
         m_rootFiber->getState() == Fiber::INIT)) {
        // 这个线程只有创建Scheduler的线程
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;
        bool temp = stopping();
        if (stopping()) {// 让其他子类有清理任务的机会
            return;
        }
    }
    // bool exit_on_this_fiber = true;
    if (m_rootThread != -1) {
        // use_caller=true的情况。
        SYLAR_ASSERT(GetThis() == this);
    } else {
        // use_caller=false的情况。
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();// 唤醒线程
    }
    if (m_rootFiber) { tickle(); }

    if (m_rootFiber) {
        if (!stopping()) { m_rootFiber->call(); }
    }
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto &i: thrs) {
        // sleep(1);
        i->join();
    }
}
void Scheduler::setThis() { t_scheduler = this; }
void Scheduler::tickle() { SYLAR_LOG_INFO(g_logger) << "tickle"; }
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);// 锁住m_fibers
    bool temp = m_autoStop && m_stopping && m_fibers.empty() &&
                m_activeThreadCount == 0;
    return temp;
}
// 没任务做就执行dile，具体做什么取决于子类，没有事情做，又不能让线程终止
void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while (!stopping()) { ytccc::Fiber::YieldToHold(); }
}

void Scheduler::run() {
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    setThis();
    if (ytccc::GetThreadID() != m_rootThread) {// 线程ID不等于主线程的ID
        t_scheduler_fiber = Fiber::GetThis().get();// 把当前协程设为主协程
    }
    Fiber::ptr idle_fiber(new Fiber([this] { idle(); }));//空闲协程
    Fiber::ptr cb_fiber; // 用于执行任务
    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;// 是否要通知其他协程，若添加新的任务则需要通知
        bool is_active = false;
        {
            // 从消息队列中取出要执行的
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // 不为初始ID并且不是期望的ID
                if (it->thread != -1 && it->thread != ytccc::GetThreadID()) {
                    ++it;
                    tickle_me = true;// 通知其他线程，让其他线程执行
                    continue;
                }
                SYLAR_ASSERT(it->fiber || it->cb);// 确定fiber或者cb存在
                // 确定状态
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }
        if (tickle_me) { tickle(); }
        // 协程 且状态不为TERM
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM &&
                         ft.fiber->getState() != Fiber::EXCEPT)) {
            // SYLAR_LOG_INFO(g_logger) << "fiber";
            ft.fiber->swapIn();// 不是结束状态，唤醒运行
            --m_activeThreadCount;
            // SYLAR_LOG_INFO(g_logger) << ft.fiber->getID() << " ft.fiber end IN";
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);// 添加到任务队列，准备再次执行
            } else if (ft.fiber->getState() != Fiber::TERM &&
                       ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->setState(Fiber::HOLD);
            }
            ft.reset();
        } else if (ft.cb) {
            // 如果是回调方法cb
            // SYLAR_LOG_INFO(g_logger) << "cb";
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {// 空指针
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                       cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);// 智能指针的reset，不会引起析构
            } else {
                cb_fiber->setState(Fiber::HOLD);
                cb_fiber.reset();
            }
        } else {
            // 为什么不直接执行idle，如果这次执行有任务，就不执行idle，直到没有任务才执行idle
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM &&
                idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
        // break;
    }
}
}// namespace ytccc
