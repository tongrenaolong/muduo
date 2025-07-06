#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include <iostream>

using namespace muduo;
using namespace muduo::net;

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        std::cout << "New connection from " << conn->peerAddress().toIpPort() << std::endl;
    } else {
        std::cout << "Connection closed from " << conn->peerAddress().toIpPort() << std::endl;
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    std::cout << "Received " << msg.size() << " bytes from " << conn->peerAddress().toIpPort() << std::endl;
    conn->send(msg);
}

int main() {
    EventLoop loop;
    InetAddress listenAddr(8888);
    TcpServer server(&loop, listenAddr, "MyServer");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    // 设置线程数量为 3
    server.setThreadNum(3);
    server.start();

    loop.loop();
    return 0;
}