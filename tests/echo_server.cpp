//
// Created by YTCCC on 2024/1/17.
//
#include "bytearray.h"
#include "iomanager.h"
#include "log.h"
#include "socket.h"
#include "tcp_server.h"

static ytccc::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public ytccc::TcpServer {
public:
    EchoServer(int type);
    void handleClient(ytccc::Socket::ptr client) override;

private:
    int m_type;
};

EchoServer::EchoServer(int type) : m_type(type) {}
void EchoServer::handleClient(ytccc::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient " << *client;
    ytccc::ByteArray::ptr ba(new ytccc::ByteArray);
    while (true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);
        // client->recv(&iovs[0], iovs.size());

        int rt = client->recv(&iovs[0], iovs.size());
        if (rt == 0) {
            SYLAR_LOG_INFO(g_logger) << "client close:" << *client;
            break;
        } else if (rt < 0) {
            SYLAR_LOG_ERROR(g_logger)
                << "client error rt=" << rt << " err=" << errno
                << " err_str=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        // getReadBuffers(iovs, 1024);不会修改位置和size
        ba->setPosition(0);// 为了toString
        // for (int i = 0; i < rt; ++i) {
        //     std::cout << ((char *) iovs[i].iov_base)[i];
        // }
        if (m_type == 1) {//text
            SYLAR_LOG_INFO(g_logger) << "\r\n" << ba->toString();
        } else {// hex
            SYLAR_LOG_INFO(g_logger) << "\r\n" << ba->toHexString();
        }
    }
}
int type = 1;
void run() {
    EchoServer::ptr es(new EchoServer(type));
    auto addr = ytccc::Address::LookupAny("0.0.0.0:8023");
    while (!es->bind(addr)) { sleep(2); }
    es->start();
}
int main(int argc, char **argv) {
    // 如果命令行参数少于2个，输出一条日志并返回
    if (argc < 2) {
        SYLAR_LOG_INFO(g_logger)
            << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    // 检查第一个命令行参数是否是"-b"，如果是，则将type设置为2
    if (!strcmp(argv[1], "-b")) { type = 2; }
    ytccc::IOManager iom(2);
    iom.schedule(run);
    return 0;
}