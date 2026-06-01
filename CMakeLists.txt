cmake_minimum_required(VERSION 3.10)
project(chat_room)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 将你的 4 个文件全部添加进来
add_executable(chat_room 
    server.cpp 
    threadpool.cpp 
    threadpool.hpp 
    chatserver.cpp 
    chatserver.hpp
)

# 聊天室项目通常需要多线程支持
find_package(Threads REQUIRED)
target_link_libraries(chat_room Threads::Threads)
