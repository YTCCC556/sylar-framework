//
// Created by YTCCC on 2023/12/7.
//

#include "timer.h"

#include <utility>
#include "util.h"

namespace ytccc {
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(std::move(cb)), m_manager(manager) {
    m_next = ytccc::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_times.find(shared_from_this());
        m_manager->m_times.erase(it);
        return true;
    }
    return false;
}
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) { return false; }
    auto it = m_manager->m_times.find(shared_from_this());
    if (it == m_manager->m_times.end()) { return false; }
    m_manager->m_times.erase(it);
    m_next = ytccc::GetCurrentMS() + m_ms;
    m_manager->m_times.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms, bool from_now) {
    // 周期相同，并且不从现在开始计算
    if (ms == m_ms && !from_now) { return true; }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    if (!m_cb) { return false; }// 如果是空的，就不用改
    auto it = m_manager->m_times.find(shared_from_this());
    if (it == m_manager->m_times.end()) { return false; }
    // TODO 这里必须引用一下，不然下面56行会出现找不到内存的情况，不清楚原因
    auto tmp = shared_from_this();
    m_manager->m_times.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = ytccc::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(tmp, lock);
    return true;
}

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
    // 首先判断是否为空
    if (!lhs && !rhs) return false;// 左右两边都是空的，返回false
    if (!lhs) return true;         // 左边是空的，返回true
    if (!rhs) return false;        // 右边是空的，返回false；
    // 其次判断时间
    if (lhs->m_next < rhs->m_next)
        return true;// 左边的执行时间小于右边的执行时间，返沪true
    if (rhs->m_next < lhs->m_next)
        return false;// 右边的执行时间小于左边的执行时间，返回false
    // 最后判断地址
    return lhs.get() < rhs.get();
}

TimerManager::TimerManager() { m_previousTime = ytccc::GetCurrentMS(); }
TimerManager::~TimerManager() {}
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    // auto it = m_times.insert(timer).first;
    // bool at_front = (it == m_times.begin());
    // lock.unlock();
    // if (at_front) {
    //     // 如果插入到最前端，执行如下函数
    //     onTimerInsertedAtFront();
    // }
    addTimer(timer, lock);
    // lock.unlock();
    return timer;
}

static void OnTimer(const std::weak_ptr<void> &weak_cond,
                    std::function<void()> cb) {
    // 用来检查其指向的对象是否仍然存在
    // 避免在定时器回调函数中形成循环引用
    std::shared_ptr<void> tmp =
            weak_cond.lock();//将 weak_cond 转换为 std::shared_ptr<void>。
    if (tmp) { cb(); }       // 转换成功，执行回调函数
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring) {
    return addTimer(
            ms, [weak_cond, cb] { return OnTimer(weak_cond, cb); }, recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_times.empty()) { return ~0ull; }// 无符号整数的最大值
    const Timer::ptr &next = *m_times.begin();
    uint64_t now_ms = ytccc::GetCurrentMS();
    if (now_ms >= next->m_next) {
        // 该执行的定时器没有执行
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}
void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
    uint64_t now_ms = ytccc::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_times.empty()) { return; }
    }
    RWMutexType::WriteLock lock(m_mutex);

    bool rollover = detectClockRollover(now_ms);
    if (!rollover && ((*m_times.begin())->m_next == now_ms)) { return; }
    Timer::ptr now_time(new Timer(now_ms));
    // 没有回滚过 取 m_times 容器中大于等于 now_time 的第一个元素的迭代器。
    auto it = rollover ? m_times.end() : m_times.lower_bound(now_time);
    while (it != m_times.end() && (*it)->m_next == now_ms) { ++it; }
    expired.insert(expired.begin(), m_times.begin(), it);
    m_times.erase(m_times.begin(), it);
    cbs.reserve(expired.size());

    for (auto &timer: expired) {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_times.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_times.empty();
}
void TimerManager::onTimerInsertedAtFront() {}
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock) {
    auto it = m_times.insert(val).first;
    bool at_front = (it == m_times.begin()) && !m_tickled;
    if (at_front) {
        m_tickled = true;
        onTimerInsertedAtFront();
    }
}
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
        // 如果当前时间小于之前时间且小于之前时间的一个小时，则认为时间回滚过
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}
}// namespace ytccc