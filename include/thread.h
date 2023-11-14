//
// Created by YTCCC on 2023/11/9.
//

#ifndef SYLAR_FRAMEWORK_THREAD_H
#define SYLAR_FRAMEWORK_THREAD_H

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <thread>

namespace ytccc {


class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};

// 信号量封装类
class Semaphore : Noncopyable {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    sem_t m_semaphore;
};
// 局部锁的模板实现，写这个模版的目的是实现RAII机制，
// RAII机制（构造时获取资源，在生命周期内控制对资源的访问，在对象析构时释放资源。）
template<class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.lock();// TODO lock方法是什么
        m_locked = true;
    }
    ~ScopedLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked;
};

// 读锁
template<class T>
struct ReadScopeLockImpl {
public:
    ReadScopeLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.rdlock();// TODO lock方法是什么
        m_locked = true;
    }
    ~ReadScopeLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked;
};

// 写锁
template<class T>
struct WriteScopeLockImpl {
public:
    WriteScopeLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopeLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked;
};

// 互斥量封装类
class Mutex : Noncopyable {
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex() { pthread_mutex_destroy(&m_mutex); }
    void lock() { pthread_mutex_lock(&m_mutex); }
    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
};

class NullMutex : Noncopyable {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}

private:
};
// 读写互斥量封装类
class RWMutex : Noncopyable {
    // 分读写锁
public:
    typedef ReadScopeLockImpl<RWMutex> ReadLock;
    typedef WriteScopeLockImpl<RWMutex> WriteLock;

    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

    void rdlock() { pthread_rwlock_rdlock(&m_lock); }
    void wrlock() { pthread_rwlock_wrlock(&m_lock); }
    void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
    pthread_rwlock_t m_lock;
};

class NullRWMutes : Noncopyable {
public:
    typedef ReadScopeLockImpl<NullRWMutes> ReadLock;
    typedef WriteScopeLockImpl<NullRWMutes> WriteLock;

    NullRWMutes() {}
    ~NullRWMutes() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

// 提升性能 自旋锁封装类
class SpinLock : Noncopyable {
public:
    typedef ScopedLockImpl<SpinLock> Lock;
    SpinLock() { pthread_spin_init(&m_mutex, 0); }
    ~SpinLock() { pthread_spin_destroy(&m_mutex); }

    void lock() { pthread_spin_lock(&m_mutex); }
    void unlock() { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

// 提升性能2.0 原子锁封装类
class CASLock : Noncopyable {
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() { m_mutex.clear(); }
    ~CASLock() {}

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(
                &m_mutex, std::memory_order_acquire))
            ;
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_acquire);
    }

private:
    volatile std::atomic_flag m_mutex;
};

class Thread : Noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string &name);// 创建
    ~Thread();                                                // 删除

    pid_t getId() const { return m_id; }
    const std::string &getName() const { return m_name; }

    void join();

    //获得当前线程，可以针对这个线程做一些操作。
    static Thread *GetThis();
    static const std::string &GetName();
    static void SetName(const std::string &name);
    static void *run(void *arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;
};

}// namespace ytccc


#endif//SYLAR_FRAMEWORK_THREAD_H
