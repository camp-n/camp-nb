#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <future>
#include <atomic>


class thread_pool{

public:

    std::vector<std::thread>workers;            //工作线程
    std::queue<std::function<void()>>tasks;     //任务队列
   
    std::condition_variable queue_not_empty;          //条件变量

    int min_threads;                            //最小线程数
    int max_threads;                            //最大线程数
    int live_threads;                           //当前活跃线程数
    int busy_threads;                           //当前忙碌线程数
    int wait_exit_threads;                      //等待销毁的线程数
    int queue_max_size;                         //任务队列最大容量
    int queue_size;                             //队列实际数量
    std::atomic<bool> shutdown{false};                //线程池是否关闭
    


    std::mutex pool_mutex;                      //线程池互斥锁

    std::mutex queue_mutex;                     //队列互斥锁

    thread_pool(int min_threads, int max_threads, int queue_max_size);                //构造函数，创建线程池

    void add_task(std::function<void()> task);   //添加任务到任务队列
       
    void thread_worker();                            //工作线程函数

    ~thread_pool();

};


#endif