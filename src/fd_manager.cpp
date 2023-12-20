//
// Created by YTCCC on 2023/12/13.
//

#include "fd_manager.h"
#include "hook.h"
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
namespace ytccc {

FdCtx::FdCtx(int fd)
    : m_isInit(false), m_isSocket(false), m_isClosed(false),
      m_userNonblock(false), m_sysNonblock(false), m_fd(fd), m_recvTimeout(-1),
      m_sendTimeout(-1) {
    init();
}
FdCtx::~FdCtx() {}

bool FdCtx::init() {
    if (m_isInit) return true;
    m_recvTimeout = -1; // 冗余
    m_sendTimeout = -1;
    struct stat fd_stat;
    if (fstat(m_fd, &fd_stat) == -1) {
        // 获取文件状态信息,获取描述符fd所指向文件的相关信息
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    if (m_isSocket) {
        // 使用 F_GETFL 命令，表示获取文件状态标志。第三个参数 0 通常不需要，因此被省略
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)) {
            // 如果flag为非阻塞状态，将文件描述符的状态设为阻塞
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }
    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}
uint64_t FdCtx::getTimeout(int type) {
    if (type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}
FdManager::FdManager() { m_datas.resize(64); }

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if (fd == -1) return nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if ((int) m_datas.size() <= fd) {
        if (!auto_create) { return nullptr; }
    } else {
        if (m_datas[fd] || !auto_create) { return m_datas[fd]; }
    }
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if (fd > (int) m_datas.size()) m_datas.resize(fd * 1.5);
    m_datas[fd] = ctx;
    return ctx;
}
void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if ((int) m_datas.size() <= fd) { return; }
    m_datas[fd].reset();
}

}// namespace ytccc