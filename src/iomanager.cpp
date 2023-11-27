//
// Created by YTCCC on 2023/11/23.
//

#include "iomanager.h"
#include "macro.h"
#include "unistd.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>

namespace ytccc {

// static ytccc::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext &
IOManager::FdContext::getContext(Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
}
void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}
void IOManager::FdContext::triggerEvent(Event event) {
    SYLAR_ASSERT(events & event);
    events = (Event) (events & ~event);
    EventContext &ctx = getContext(event);
    // 触发后，将要执行的方法放入scheduler中
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }

    ctx.scheduler = nullptr;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(rt);

    contextResize(64);
    start();
}
IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (auto &i: m_fdContexts) { delete i; }
}

// 1 success,0 retry,-1 error
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext *fd_context = nullptr;
    RWMutexType::WriteLock lock(m_mutex);
    if (m_fdContexts.size() > fd) {
        fd_context = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::ReadLock lock(m_mutex);
        contextResize(m_fdContexts.size() * 1.5);
        fd_context = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_context->mutex);
    if (fd_context->events & event) {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                  << " fd_ctx.event=" << fd_context->events;
        SYLAR_ASSERT(!(fd_context->events & event));
    }
    int op = fd_context->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_context | event;
    epevent.data.ptr = fd_context;

    // 将事件加入epoll中
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
                << "epoll_ctl(" << m_epfd << "," << op << "," << fd << ","
                << epevent.events << "):" << rt << " (" << errno << ") ("
                << strerror(errno) << ")";
        return -1;
    }
    ++m_pendingEventCount;
    fd_context->events = (Event) (fd_context | event);
    FdContext::EventContext &event_ctx = fd_context->getContext(event);
    // 三个值都应该是无效的
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        context.cb.swap(cb);
    } else {
        context.fiber = Fiber::GetThis();
        SYLAR_ASSERT(context.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= fd) { return false; }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock1(fd_ctx->mutex);
    if (fd_ctx->events & event) { return false; }
    Event new_event = (Event) (fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
                << "epoll_ctl(" << m_epfd << "," << op << "," << fd << ","
                << epevent.events << "):" << rt << " (" << errno << ") ("
                << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= fd) { return false; }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock1(fd_ctx->mutex);
    if (fd_ctx->events & event) { return false; }
    Event new_event = (Event) (fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
                << "epoll_ctl(" << m_epfd << "," << op << "," << fd << ","
                << epevent.events << "):" << rt << " (" << errno << ") ("
                << strerror(errno) << ")";
        return false;
    }
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}//和删除不同，取消条件，强制执行
bool IOManager::cancelAll(int fd) {
    // 取消该句柄下所有事件
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= fd) { return false; }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock1(fd_ctx->mutex);
    // 判断是否有事件
    if (fd_ctx->events) { return false; }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
                << "epoll_ctl(" << m_epfd << "," << op << "," << fd << ","
                << epevent.events << "):" << rt << " (" << errno << ") ("
                << strerror(errno) << ")";
        return false;
    }
    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    SYLAR_ASSERT(fd_ctx->events == 0)
    return true;
}

IOManager *IOManager::GetThis() {
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if (hasIdleThreads()) { return; }
    int rt = write(m_tickleFds[1], "T", 1);//写入返回写入的长度
    SYLAR_ASSERT(rt == 1);
}
bool IOManager::stopping() {
    return Scheduler::stopping() && m_pendingEventCount == 0;
}
void IOManager::idle() {
    epoll_event *events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(
            events, [events](epoll_event *ptr) { delete[] events; });
    while (true) {
        if (stopping()) {
            SYLAR_LOG_INFO(g_logger)
                    << "name=" << m_name << " idle stopping exit";
            break;
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 5000;// 设置超时时间
            rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);
            if (rt && errno == EINTR) {
                // 返回值小于0
            } else {
                break;
            }
        } while (true);

        for (int i = 0; i < rt; ++i) {
            epoll_event &event = events[i];
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while (read(m_tickleFds[0], &dummy, 1) == 1) { continue; }
            }

            FdContext *fd_ctx = event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) { real_events |= READ; }
            if (event.events & EPOLLOUT) { real_events |= WRITE; }
            if ((fd_ctx->events & real_events) == NONE) { continue; }
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                SYLAR_LOG_ERROR(g_logger)
                        << "epoll_ctl(" << m_epfd << "," << op << ","
                        << fd_ctx->fd << "," << event.events << "):" << rt2
                        << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}
void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    for (int i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

}// namespace ytccc
