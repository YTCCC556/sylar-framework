//
// Created by YTCCC on 2023/12/7.
//

#ifndef SYLAR_FRAMEWORK_TIMER_H
#define SYLAR_FRAMEWORK_TIMER_H

#include "thread.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>


namespace ytccc {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancel();
    bool refresh();
    bool reset(uint64_t ms, bool from_now);

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager *manager);
    Timer(uint64_t next);



private:
    bool m_recurring = false;// 是否循环执行器
    uint64_t m_ms = 0;       // 执行周期
    uint64_t m_next = 0;     // 精确的执行时间
    TimerManager *m_manager = nullptr;
    std::function<void()> m_cb;

private:
    struct Comparator {
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

class TimerManager {
    friend class Timer;

public:
    typedef RWMutex RWMutexType;
    TimerManager();
    virtual ~TimerManager();
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond,
                                 bool recurring = false);
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void()>> &cbs);
    bool hasTimer();
protected:
    virtual void onTimerInsertedAtFront();
    void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

private:
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_times;
    bool m_tickled = false;
    uint64_t m_previousTime = 0;
};
}// namespace ytccc


#endif//SYLAR_FRAMEWORK_TIMER_H
