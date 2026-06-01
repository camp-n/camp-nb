#pragma once
#include <vector>
#include <iostream>
#include <queue>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <functional>
#include <future>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include"threadpool.hpp"

#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define SERVER_POST 8080

class chatserver{
public:
    chatserver();
    
    ~chatserver();

    int listen_fd;
    int epoll_fd;
    thread_pool worker_pool;
    
    void listener_init();

    void listener_routine(thread_pool& thread_pool);

    void listener_start(thread_pool& thread_pool);

    void manager_routine(thread_pool& thread_pool);

    std::thread listener_thread;
    std::thread manager_thread;
};

