cmake_minimum_required(VERSION 3.10)
project(EchoServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)

# 查找 muduo 库
find_package(muduo REQUIRED)
include_directories(${MUDUO_INCLUDE_DIRS})

# 添加可执行文件
add_executable(echo_server server.cpp)
add_executable(echo_client client.cpp)

# 链接 muduo 库
target_link_libraries(echo_server ${MUDUO_LIBRARIES} pthread)
target_link_libraries(echo_client ${MUDUO_LIBRARIES} pthread)