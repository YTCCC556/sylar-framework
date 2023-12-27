//
// Created by YTCCC on 2023/12/13.
//

#include "hook.h"
#include "config.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/socket.h>

ytccc::Logger::ptr hook_logger = SYLAR_LOG_NAME("system");

namespace ytccc {

static ytccc::ConfigVar<int>::ptr g_tcp_timeout = ytccc::Config::Lookup(
        "tcp.connect.timeout", 5000, "tcp connect timeout");
static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)                                                           \
    XX(sleep)                                                                  \
    XX(usleep)                                                                 \
    XX(nanosleep)                                                              \
    XX(socket)                                                                 \
    XX(connect)                                                                \
    XX(accept)                                                                 \
    XX(read)                                                                   \
    XX(readv)                                                                  \
    XX(recv)                                                                   \
    XX(recvfrom)                                                               \
    XX(recvmsg)                                                                \
    XX(write)                                                                  \
    XX(writev)                                                                 \
    XX(send)                                                                   \
    XX(sendto)                                                                 \
    XX(sendmsg)                                                                \
    XX(close)                                                                  \
    XX(fcntl)                                                                  \
    XX(ioctl)                                                                  \
    XX(getsockopt)                                                             \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if (is_inited) { return; }
#define XX(name) name##_f = (name##_fun) dlsym(RTLD_NEXT, #name);
    // dlsym函数用于在动态链接库中查找符号
    HOOK_FUN(XX);
#undef XX
}
// 在main之前运行
static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_timeout->getValue();
        g_tcp_timeout->addListener([](const int &old_value,
                                      const int &new_value) {
            SYLAR_LOG_INFO(hook_logger) << "tcp connect timeout changed from "
                                        << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};
static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }
void set_hook_enable(bool flag) { t_hook_enable = flag; }

}// namespace ytccc

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
    if (!ytccc::t_hook_enable) return fun(fd, std::forward<Args>(args)...);
    // 判断是否进入
    // SYLAR_LOG_DEBUG(hook_logger) << "do_io<" << hook_fun_name << ">";

    ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(fd);
    if (!ctx) { return fun(fd, std::forward<Args>(args)...); }
    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    if (!ctx->isSocked() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);// 智能指针

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);// 调用系统函数
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if (n == -1 && errno == EAGAIN) {
        // 判断是否进入
        // SYLAR_LOG_DEBUG(hook_logger) << "do_io<" << hook_fun_name << ">";
        ytccc::IOManager *iom = ytccc::IOManager::GetThis();
        ytccc::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(
                tinfo);// 避免在定时器回调函数中形成循环引用
        if (to != (uint64_t) -1) {
            // TODO 条件定时器就是用来取消事件的吗
            timer = iom->addConditionTimer(
                    to,
                    [winfo, fd, iom, event]() {
                        auto t = winfo.lock();
                        if (!t || t->cancelled) {
                            return;
                        }// 已经被取消了，直接返回
                        // 否则，设置标志，取消事件
                        t->cancelled = ETIMEDOUT;
                        iom->cancelEvent(fd, (ytccc::IOManager::Event)(event));
                    },
                    winfo);
        }

        int rt = iom->addEvent(fd, (ytccc::IOManager::Event)(event));
        if (rt) {// 注册事件失败
            /*if (c) {
                SYLAR_LOG_ERROR(g_logger)
                        << hook_fun_name << " addEvent(" << fd << "," << event
                        << ") retry c=" << c
                        << " used=" << (ytccc::GetCurrentMS() - now);
            }*/
            SYLAR_LOG_ERROR(hook_logger) << hook_fun_name << " addEvent(" << fd
                                         << "," << event << ")";
            if (timer) { timer->cancel(); }
            return -1;
        } else {
            ytccc::Fiber::YieldToHold();// 让出执行权，等待事件发生
            // 创建一个定时器，同时注册一个事件，并让当前协程让出执行权，等待事件发生。
            if (timer) { timer->cancel(); }
            if (tinfo->cancelled) {// 定时器已经被取消
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}


extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

// sleep
unsigned int sleep(unsigned int seconds) {
    if (!ytccc::t_hook_enable) { return sleep_f(seconds); }
    ytccc::Fiber::ptr fiber = ytccc::Fiber::GetThis();
    ytccc::IOManager *iom = ytccc::IOManager::GetThis();
    // iom->addTimer(seconds * 1000,
    //               std::bind((void(ytccc::Scheduler::*)(ytccc::Fiber::ptr,
    //                                                    int thread)) &
    //                                 ytccc::IOManager::schedule,
    //                         iom, fiber, -1));
    // iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber); });
    iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
    ytccc::Fiber::YieldToHold();
    return 0;
}
int usleep(useconds_t usec) {
    if (!ytccc::t_hook_enable) { return usleep_f(usec); }
    ytccc::Fiber::ptr fiber = ytccc::Fiber::GetThis();
    ytccc::IOManager *iom = ytccc::IOManager::GetThis();

    // iom->addTimer(usec / 1000,
    //               std::bind((void(ytccc::Scheduler::*)(ytccc::Fiber::ptr,
    //                                                    int thread)) &
    //                                 ytccc::IOManager::schedule,
    //                         iom, fiber, -1));
    iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
    ytccc::Fiber::YieldToHold();
    return 0;
}
int nanosleep(const struct timespec *reg, struct timespec *rem) {
    if (!ytccc::t_hook_enable) { return nanosleep_f(reg, rem); }
    int timeout_ms = reg->tv_sec * 1000 + reg->tv_nsec / 1000 / 1000;

    ytccc::Fiber::ptr fiber = ytccc::Fiber::GetThis();
    ytccc::IOManager *iom = ytccc::IOManager::GetThis();
    // iom->addTimer(timeout_ms,
    //               std::bind((void(ytccc::Scheduler::*)(ytccc::Fiber::ptr,
    //                                                    int thread)) &
    //                                 ytccc::IOManager::schedule,
    //                         iom, fiber, -1));
    iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber, -1); });
    ytccc::Fiber::YieldToHold();
    return 0;
}

//socket
int socket(int domain, int type, int protocol) {
    if (!ytccc::t_hook_enable) { return socket_f(domain, type, protocol); }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) return fd;
    ytccc::FdMgr::GetInstance()->get(fd, true);
    return fd;
}
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
    if (!ytccc::t_hook_enable) { return connect_f(fd, addr, addrlen); }
    ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    if (!ctx->isSocked()) { return connect_f(fd, addr, addrlen); }
    if (ctx->getUserNonblock()) { return connect_f(fd, addr, addrlen); }
    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {
        // 没有出错，没有正在执行
        return n;
    }
    ytccc::IOManager *iom = ytccc::IOManager::GetThis();
    ytccc::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);
    if (timeout_ms != (uint64_t) -1) {
        timer = iom->addConditionTimer(
                timeout_ms,
                [winfo, fd, iom]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) { return; }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, ytccc::IOManager::WRITE);
                },
                winfo);
    }
    int rt = iom->addEvent(fd, ytccc::IOManager::WRITE);
    if (rt == 0) {
        // 添加成功
        ytccc::Fiber::YieldToHold();// 让出控制权，等待唤醒
        if (timer) { timer->cancel(); }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) { timer->cancel(); }
        SYLAR_LOG_ERROR(hook_logger)
                << "connect addEvent (" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) return -1;
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // return connect_f(sockfd, addr, addrlen);
    return connect_with_timeout(sockfd, addr, addrlen,
                                ytccc::s_connect_timeout);
}
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", ytccc::IOManager::READ, SO_RCVTIMEO,
                   addr, addrlen);
    if (fd >= 0) { ytccc::FdMgr::GetInstance()->get(fd, true); }
    return fd;
}

//read
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", ytccc::IOManager::READ, SO_RCVTIMEO, buf,
                 count);
}
ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", ytccc::IOManager::READ, SO_RCVTIMEO, iov,
                 iovcnt);
}
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", ytccc::IOManager::READ, SO_RCVTIMEO,
                 buf, len, flags);
}
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", ytccc::IOManager::READ,
                 SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", ytccc::IOManager::READ,
                 SO_RCVTIMEO, msg, flags);
}

//write
ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", ytccc::IOManager::WRITE, SO_SNDTIMEO,
                 buf, count);
}
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", ytccc::IOManager::WRITE, SO_SNDTIMEO,
                 iov, iovcnt);
}
ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", ytccc::IOManager::WRITE, SO_SNDTIMEO, msg,
                 len, flags);
}
ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", ytccc::IOManager::WRITE, SO_SNDTIMEO,
                 msg, len, flags, to, tolen);
}
ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", ytccc::IOManager::WRITE, SO_SNDTIMEO,
                 msg, flags);
}
int close(int fd) {
    if (!ytccc::t_hook_enable) { return close_f(fd); }
    ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        auto iom = ytccc::IOManager::GetThis();
        if (iom) { iom->cancelAll(fd); }
        ytccc::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ...) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
        case F_SETFL: {
            int arg = va_arg(va, int);
            va_end(va);
            ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocked()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETFL: {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocked()) { return arg; }
            if (ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
        }
            // int
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ: {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            // void

        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ: {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
            //flock
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK: {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            //f_owner_exlock
        case F_GETOWN_EX:
        case F_SETOWN_EX: {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}
int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void *);
    va_end(va);
    if (FIONBIO == request) {
        bool user_nonblock = !!*(int *) arg;
        ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(d);
        if (!ctx || ctx->isClose() || !ctx->isSocked()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}
int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}
int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
    if (!ytccc::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            ytccc::FdCtx::ptr ctx = ytccc::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval *v = (const timeval *) optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
