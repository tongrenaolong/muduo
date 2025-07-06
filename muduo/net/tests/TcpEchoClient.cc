#include "muduo/net/TcpClient.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include <iostream>
#include "muduo/base/CurrentThread.h"

using namespace muduo;
using namespace muduo::net;

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        std::cout << "Connected to server" << std::endl;
        conn->send("Hello, server!");
    } else {
        std::cout << "Disconnected from server" << std::endl;
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
    std::cout << "Thread ID: " << muduo::CurrentThread::tid() << " "
              << "Thread ID: " << conn->getLoop()->threadId() << " "
              << ", Loop Address: " << conn->getLoop() << std::endl;
    std::string msg(buf->retrieveAllAsString());
    std::cout << "Received from server: " << msg << std::endl;
    getchar();
    conn->send(msg);
}

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    TcpClient client(&loop, serverAddr, "MyClient");

    client.setConnectionCallback(onConnection);
    client.setMessageCallback(onMessage);
    client.connect();
    loop.loop();
    return 0;
}