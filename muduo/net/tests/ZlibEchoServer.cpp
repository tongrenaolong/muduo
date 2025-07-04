#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/Buffer.h"
#include "muduo/base/Logging.h"
#include "muduo/net/ZlibStream.h"

using namespace muduo;
using namespace muduo::net;

class ZlibEchoServer {
private:
    EventLoop* loop_;
    TcpServer server_;

    void onConnection(const TcpConnectionPtr& conn) {
        LOG_INFO << "Connection " << (conn->connected() ? "UP" : "DOWN");
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        // 解压数据
        Buffer uncompressed;
        ZlibInputStream inputStream(&uncompressed);
        if (!inputStream.write(buf)) {
            LOG_ERROR << "Decompression failed";
            return;
        }
        inputStream.finish();

        // 重新压缩数据
        Buffer compressed;
        ZlibOutputStream outputStream(&compressed);
        if (!outputStream.write(uncompressed.peek(), uncompressed.readableBytes())) {
            LOG_ERROR << "Compression failed";
            return;
        }
        outputStream.finish();

        // 回显数据
        conn->send(&compressed);
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
    InetAddress listenAddr(8080);
    ZlibEchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
}