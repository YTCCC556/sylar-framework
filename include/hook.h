//
// Created by YTCCC on 2023/12/13.
//

#ifndef YTCCC_MODULE_HOOK_H
#define YTCCC_MODULE_HOOK_H

#include <csignal>
#include <cstdint>
#include <cstdlib>
namespace ytccc {
bool is_hook_enable();
void set_hook_enable(bool flag);

}// namespace ytccc

extern "C" {
/* typedef unsigned int (*sleep_fun)(unsigned int seconds);
 * 定义类型别名，表示一个指针函数类型，是一个指向有一个参数的函数的指针。
 * */

// sleep 相关
typedef unsigned int (*sleep_fun)(unsigned int seconds);// 声明函数指针类型
typedef int (*usleep_fun)(useconds_t usec);
typedef int (*nanosleep_fun)(const struct timespec *reg, struct timespec *rem);

extern sleep_fun sleep_f;// 声明外部函数指针变量
extern usleep_fun usleep_f;
extern nanosleep_fun nanosleep_f;

//socket
typedef int (*socket_fun)(int domain, int type, int protocol);
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr,
                           socklen_t addrlen);
typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);

extern socket_fun socket_f;
extern connect_fun connect_f;
extern accept_fun accept_f;

//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);

extern read_fun read_f;
extern readv_fun readv_f;
extern recv_fun recv_f;
extern recvfrom_fun recvfrom_f;
extern recvmsg_fun recvmsg_f;

//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*send_fun)(int s, const void *msg, size_t len, int flags);
typedef ssize_t (*sendto_fun)(int s, const void *msg, size_t len, int flags,
                          const struct sockaddr *to, socklen_t tolen);
typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
typedef int (*close_fun)(int fd);

extern write_fun write_f;
extern writev_fun writev_f;
extern send_fun send_f;
extern sendto_fun sendto_f;
extern sendmsg_fun sendmsg_f;
extern close_fun close_f;

//
typedef int (*fcntl_fun)(int fd, int cmd, ...);
typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval,
                              socklen_t *optlen);
typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);

extern fcntl_fun fcntl_f;
extern ioctl_fun ioctl_f;
extern getsockopt_fun getsockopt_f;
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

}


#endif//YTCCC_MODULE_HOOK_H
