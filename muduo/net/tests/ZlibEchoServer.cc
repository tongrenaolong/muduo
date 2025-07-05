#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/ZlibStream.h"
#include "muduo/base/Logging.h"

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
using namespace muduo;
using namespace muduo::net;
#include <iostream>
using namespace std;

class ZlibEchoServer {
private:
    EventLoop* loop_;
    TcpServer server_;

    void onConnection(const TcpConnectionPtr& conn) {
        LOG_INFO << "Connection " << (conn->connected() ? "UP" : "DOWN");
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        // 解压数据
        Buffer decompressed;
        ZlibInputStream inputStream(&decompressed);
        // 将外部数据加压保存在内部 
        if (!inputStream.write(buf)) {
            LOG_ERROR << "Decompression failed";
            return;
        }
        inputStream.finish();
    }
public:
    ZlibEchoServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop), server_(loop, listenAddr, "ZlibEchoServer") {
        server_.setConnectionCallback(std::bind(&ZlibEchoServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(&ZlibEchoServer::onMessage, this, _1, _2, _3));
    }

    void start() {
        server_.start();
    }
};

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress listenAddr(8000);
    ZlibEchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
}