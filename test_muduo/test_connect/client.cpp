#include "muduo/net/TcpClient.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"

#include <iostream>

using namespace muduo;
using namespace muduo::net;

class EchoClient {
private:
    EventLoop* loop_;
    TcpClient client_;

public:
    EchoClient(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop), client_(loop, serverAddr, "EchoClient") {
        client_.setConnectionCallback(std::bind(&EchoClient::onConnection, this, _1));
        client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
    }

    void connect() {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO << "Connected to server";
            conn->send("Hello, Server!\n");
        } else {
            LOG_INFO << "Disconnected from server";
            loop_->quit();
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        string msg(buf->retrieveAllAsString());
        LOG_INFO << "Received from server: " << msg;
        conn->shutdown();
    }
};

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8080);
    EchoClient client(&loop, serverAddr);
    client.connect();
    loop.loop();
    return 0;
}