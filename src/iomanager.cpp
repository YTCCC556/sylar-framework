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
    // 只有events中包含event时才能通过此语句，用于检查对应位是否被设置
    SYLAR_ASSERT(events & event);
    // 用于从一组标志中移除特定的标志，从events中去除event中的标志
    events = (Event) (events & ~event);
    // 获得事件的类型
    EventContext &ctx = getContext(event);
    // 触发后，将要执行的方法放入scheduler中
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    // 执行完毕，设空
    ctx.scheduler = nullptr;
    // return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
    // 创建了一个 epoll 实例，并将其文件描述符保存在 m_epfd 变量中
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    // 创建一个管道，结果保存在m_tickleFds数组中，
    // 通常，m_tickleFds[0] 用于读取，m_tickleFds[1] 用于写入。
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    // 创建epoll_event结构体，并将其初始化为0
    epoll_event event;
    // memset 将 ptr 指向的内存区域的前 num 个字节都设置为 value。这在初始化数组或清除内存时非常有用
    memset(&event, 0, sizeof(epoll_event));
    // 文件类型为可读，可写
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];
    // 将管道的读取端设置为非阻塞模式
    // 非阻塞模式（没有数据可用，读取操作不会阻塞进程，而是立即返回一个错误或者0字节）
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);
    // 将文件描述符添加到epoll实例中，监听可读事件，
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

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
    RWMutexType::ReadLock lock(m_mutex);
    if ((int) m_fdContexts.size() > fd) {
        fd_context = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_context = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_context->mutex);
    if (fd_context->events & event) {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                  << " fd_ctx.event=" << fd_context->events;
        SYLAR_ASSERT(!(fd_context->events & event));
    }
    // 有事件 则op被设为MOD，没有事件被设为ADD
    int op = fd_context->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_context->events | event;
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
    fd_context->events = (Event) (fd_context->events | event);
    FdContext::EventContext &event_ctx = fd_context->getContext(event);
    // 三个值都应该是无效的
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                      "state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int) m_fdContexts.size() <= fd) { return false; }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock1(fd_ctx->mutex);
    if (fd_ctx->events & event) { return false; }
    Event new_event = (Event) (fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    // EPOLLET 用于设置epoll事件的触发方式，
    // EPOLLET表示边缘触发模式，只在状态变化的瞬间触发一次，而不是持续触发，必须使用非阻塞文件描述符
    // 如不设置EPOLLET则默认为水平触发模式，当文件描述符就为时，不断触发，直到处理完所有就绪事件。
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
    if ((int) m_fdContexts.size() <= fd) { return false; }
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
    if ((int) m_fdContexts.size() <= fd) { return false; }
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
    if (!hasIdleThreads()) { return; }
    ssize_t rt = write(m_tickleFds[1], "T", 1);//写入返回写入的长度
    SYLAR_ASSERT(rt == 1);
}
bool IOManager::stopping() {
    return Scheduler::stopping() && m_pendingEventCount == 0;
}
void IOManager::idle() {
    SYLAR_LOG_DEBUG(g_logger) << "io manager idle";
    //用智能指针包装数组，确保在不再需要这个数组时能够自动释放其内存。
    epoll_event *events = new epoll_event[64]();
    // 第二个参数时lambda函数，定义了智能指针引用计数归零时应该调用的删除器
    std::shared_ptr<epoll_event> shared_events(
            events, [](epoll_event *ptr) { delete[] ptr; });

    while (true) {
        if (stopping()) {
            SYLAR_LOG_INFO(g_logger) << "idle stopping exit "
                                     << "name=" << getName();
            break;
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;// 设置超时时间
            //如果有事件发生，函数会填充events数组，返回发生事件的文件描述符的数量
            rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);
            SYLAR_LOG_INFO(g_logger) << "epoll_wait rt=" << rt;
            if (rt < 0 && errno == EINTR) {
                // 出错，返回值-1，并设置errno指示出错原因
            } else {
                // 正常返回发生事件的文件描述符的数量或者超时返回0。
                break;
            }
        } while (true);

        for (int i = 0; i < rt; ++i) {
            epoll_event &event = events[i];
            // SYLAR_LOG_INFO(g_logger) << event.data.fd;
            // 发生事件的文件描述符是否为m_tickleFds[0]，也就是读事件
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while (read(m_tickleFds[0], &dummy, sizeof(dummy)) > 0) {
                    continue;
                }
            }
            // 获得事件对应的事件描述符内容
            FdContext *fd_ctx = (FdContext *) event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // EPOLLERR 表示出现错误 EPOLLHUP 发生挂起事件
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                // 如果发生了错误或挂起事件，就将事件集中的EPOLLIN和EPOLLOUT设为fd_ctx->events中相应的标志。
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                /* EPOLLIN|EPOLLOUT可以理解为读写事件
                 * 读写事件 & 实际事件集 表示实际事件集中读写事件的设置
                 * */
            }
            int real_events = NONE; // 实际发生的事件
            if (event.events & EPOLLIN) { real_events |= READ; }
            if (event.events & EPOLLOUT) { real_events |= WRITE; }
            // 文件描述符关注的事件，和实际发生的事件
            if ((fd_ctx->events & real_events) == NONE) { continue; }
            int left_events = (fd_ctx->events & ~real_events); // 去除发生的事件
            // 是否还剩下事件
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
