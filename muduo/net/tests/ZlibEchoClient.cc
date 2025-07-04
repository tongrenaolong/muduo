#include "muduo/net/TcpClient.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/Buffer.h"
#include "muduo/base/Logging.h"
#include "muduo/net/ZlibStream.h"
#include <iostream>

using namespace muduo;
using namespace muduo::net;

class ZlibEchoClient {
private:
    EventLoop* loop_;
    TcpClient client_;

    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            std::cout << "Connected to server, please input message: " << std::endl;
            std::string message;
            std::getline(std::cin, message);

            // 压缩数据
            Buffer compressed;
            ZlibOutputStream outputStream(&compressed);
            if (!outputStream.write(message)) {
                LOG_ERROR << "Compression failed";
                return;
            }
            outputStream.finish();

            conn->send(&compressed);
        }
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

        std::cout << "Received from server: " << uncompressed.retrieveAllAsString() << std::endl;
        conn->shutdown();
    }

public:
    ZlibEchoClient(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop), client_(loop, serverAddr, "ZlibEchoClient") {
        client_.setConnectionCallback(std::bind(&ZlibEchoClient::onConnection, this, _1));
        client_.setMessageCallback(std::bind(&ZlibEchoClient::onMessage, this, _1, _2, _3));
    }

    void connect() {
        client_.connect();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s server_ip\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress serverAddr(argv[1], 8000);
    ZlibEchoClient client(&loop, serverAddr);
    client.connect();
    loop.loop();
    return 0;
}