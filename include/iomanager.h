//
// Created by YTCCC on 2023/11/23.
//

#ifndef SYLAR_FRAMEWORK_IOMANAGER_H
#define SYLAR_FRAMEWORK_IOMANAGER_H

#include "scheduler.h"

namespace ytccc {

class IOManager : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    enum Event { NONE = 0x0, READ = 0x1, WRITE = 0x4 };

private:
    // 类的外部和派生类都不可见
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler *scheduler = nullptr;// 事件执行的scheduler
            Fiber::ptr fiber;              // 事件协程
            std::function<void()> cb;      //回调函数
        };
        EventContext &getContext(Event event);
        void resetContext(EventContext &ctx);
        void triggerEvent(Event event);

        int fd = 0;         // 事件关联句柄
        EventContext read;  // 读事件
        EventContext write; // 写事件
        Event events = NONE;// 已注册的事件集合
        MutexType mutex;    //锁
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true,
              const std::string &name = "");
    ~IOManager();
    // 0 success,-1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);//和删除不同，取消条件，强制执行
    bool cancelAll(int fd);
    static IOManager *GetThis();

protected:
    // 外部不可见，派生类可见
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void contextResize(size_t size);

private:
    int m_epfd = 0; // epoll文件句柄
    int m_tickleFds[2]; // pipe文件句柄

    std::atomic<size_t> m_pendingEventCount = 0; // 当前等待执行事件数量
    RWMutexType m_mutex;
    std::vector<FdContext *> m_fdContexts; //socket事件上下文容器
};

}// namespace ytccc


#endif//SYLAR_FRAMEWORK_IOMANAGER_H
