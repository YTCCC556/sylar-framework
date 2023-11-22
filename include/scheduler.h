//
// Created by YTCCC on 2023/11/20.
//

#ifndef SYLAR_FRAMEWORK_SCHEDULER_H
#define SYLAR_FRAMEWORK_SCHEDULER_H

#include <utility>

#include "fiber.h"
#include "functional"
#include "list"
#include "log.h"
#include "macro.h"
#include "memory"
#include "thread.h"
#include "vector"

namespace ytccc {
static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string &name =
                      "");// use_caller 是否将caller线程纳入线程调度器
    virtual ~Scheduler();

    const std::string &getName() const { return m_name; }

    static Scheduler *GetThis();
    static Fiber *GetMainFiber();

    void start();
    void stop();

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) { tickle(); }
    };

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) { tickle(); }
    }

protected:
    virtual void tickle();
    virtual bool stopping();
    // 没任务做就执行dile，具体做什么取决于子类，没有事情做，又不能让线程终止
    virtual void idle();
    void run();
    void setThis();

private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();// 若为空，则需要通知
        // 新建任务，添加到任务队列
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            if (ft.fiber) {
                SYLAR_LOG_INFO(g_logger)
                        << ft.fiber->getID() << " fiber added to Task Queue";
            } else {
                SYLAR_LOG_INFO(g_logger) << "cb added to Task Queue";
            }
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(std::move(f)), thread(thr) {}
        FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }
        FiberAndThread(std::function<void()> f, int thr)
            : cb(std::move(f)), thread(thr) {}
        FiberAndThread(std::function<void()> *f, int thr) : thread(thr) {
            cb.swap(*f);
        }
        FiberAndThread() : thread(-1) {
            // 默认构造函数，stl的一些东西，分配对象一定需要个默认构造函数，否者分配的对象无法初始化。
        }
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;// 任务队列
    Fiber::ptr m_rootFiber;// use_caller=true时有效，调度协程
    std::string m_name;

protected:
    std::vector<int> m_threadIDs;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true; // 是否停止，表示执行状态
    bool m_autoStop = false;// 是否主动停止
    int m_rootThread = 0;   // 主线程ID
};
}// namespace ytccc


#endif//SYLAR_FRAMEWORK_SCHEDULER_H
