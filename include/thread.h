//
// Created by YTCCC on 2023/11/9.
//

#ifndef SYLAR_FRAMEWORK_THREAD_H
#define SYLAR_FRAMEWORK_THREAD_H

#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <thread>

namespace ytccc {

class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    Semaphore(const Semaphore &) = delete;           // 删除拷贝构造函数
    Semaphore(const Semaphore &&) = delete;          // 删除移动构造函数
    Semaphore &operator=(const Semaphore &) = delete;//删除拷贝赋值运算符

private:
    sem_t m_semaphore;
};

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
        m_mutex.wrlock();// TODO lock方法是什么
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

class Mutex {
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex() { pthread_mutex_destroy(&m_mutex); }
    void lock() { pthread_mutex_lock(&m_mutex); }
    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
};

class NullMutex {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}

private:
};

class RWMutex {
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

class NullRWMutes {
public:
    typedef ReadScopeLockImpl<NullRWMutes> ReadLock;
    typedef WriteScopeLockImpl<NullRWMutes> WriteLock;

    NullRWMutes() {}
    ~NullRWMutes() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

// 提升性能
class SpinLock {
public:
    typedef ScopedLockImpl<SpinLock> Lock;
    SpinLock() { pthread_spin_init(&m_mutex, 0); }
    ~SpinLock() { pthread_spin_destroy(&m_mutex); }

    void lock() { pthread_spin_lock(&m_mutex); }
    void unlock() { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string &name);
    ~Thread();
    pid_t getId() const { return m_id; }
    const std::string &getName() const { return m_name; }

    void join();

    //获得当前线程，可以针对这个线程做一些操作。
    static Thread *GetThis();
    static const std::string &GetName();
    static void SetName(const std::string &name);
    static void *run(void *arg);

private:
    //防止复制？
    Thread(const Thread &) = delete;           // 删除拷贝构造函数
    Thread(const Thread &&) = delete;          // 删除移动构造函数
    Thread &operator=(const Thread &) = delete;// 删除拷贝赋值运算符

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;
};


}// namespace ytccc


#endif//SYLAR_FRAMEWORK_THREAD_H
