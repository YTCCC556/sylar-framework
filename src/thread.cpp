//
// Created by YTCCC on 2023/11/9.
//

#include "thread.h"
#include "log.h"
#include "util.h"

namespace ytccc {

//Semaphore实现
Semaphore::Semaphore(uint32_t count) {
    if (sem_init(&m_semaphore, 0, count)) {
        // 对由sem指定的信号量进行初始化，设置好共享选项，指定证书类型的初始值，
        // 若pshared=0，表示它是当前进程的局部信号量，否则，其他进程就能共享这个信号量
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    // 释放信号量，在用完信号量后对他进行清理
    sem_destroy(&m_semaphore); }

void Semaphore::wait() {
    // 阻塞当前线程，直到信号量sem的值大于0，解除阻塞后将sem的值减一。
    if (sem_wait(&m_semaphore)) { throw std::logic_error("sem_wait error"); }
}

void Semaphore::notify() {
    // 增加信号量sem的值，
    if (sem_post(&m_semaphore)) { throw std::logic_error("sem_post error"); }
}


//Thread实现

//获得当前线程，可以针对这个线程做一些操作。
static thread_local Thread *t_thread = nullptr;//线程局部变量
static thread_local std::string t_thread_name = "UNKNOWN";

static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Thread *Thread::GetThis() { return t_thread; }
const std::string &Thread::GetName() { return t_thread_name; }
void Thread::SetName(const std::string &name) {
    if (t_thread) { t_thread->m_name = name; }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(std::move(cb)), m_name(name) {
    if (name.empty())  m_name = "UNKNOWN";
    // 创建线程
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail,rt = " << rt
                                  << " name= " << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}
Thread::~Thread() {
    if (m_thread) { pthread_detach(m_thread); }
}

void Thread::join() {
    if (m_thread) {
        // 获得线程执行结束时返回的数据
        // 常见场景是在主线程中等待子线程的结束。这可以确保主线程在子线程完成任务之前不会提前退出
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail,rt = " << rt
                                      << " name:" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void *Thread::run(void *arg) {
    // void指针参数强制转换成Thread指针
    Thread *thread = (Thread *) arg;
    // 设置线程和线程名称以备后续使用
    t_thread = thread;
    t_thread_name = thread->m_name;
    // 设置线程id
    thread->m_id = ytccc::GetThreadID();
    // 为线程设置名称，最大字符15
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb); // 交换两个std::function对象的内容

    thread->m_semaphore.notify(); // 通知信号量，表示线程准备就绪

    cb(); // 执行回调函数
    return 0;
}
}// namespace ytccc
