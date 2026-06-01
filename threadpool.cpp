#include"threadpool.hpp"

thread_pool::thread_pool(int min_threads, int max_threads, int queue_max_size){
    this->min_threads = min_threads;
    this->max_threads = max_threads;
    this->queue_max_size = queue_max_size;
    this->live_threads = 0;
    this->busy_threads = 0;
    this->queue_size = 0;
    this->wait_exit_threads = 0;

    for(int i=0;i<min_threads;++i){
       workers.emplace_back(&thread_pool::thread_worker,this); //创建线程
       live_threads++;
    }
}


void thread_pool::add_task(std::function<void()> task){      //添加任务到任务队列
    {
        std::unique_lock<std::mutex> lock(queue_mutex);      //锁住任务队列
        tasks.push(std::move(task));
        queue_size++;
    }
    queue_not_empty.notify_one();              //唤醒一个等待的线程
}

void thread_pool::thread_worker(){        //工作线程函数
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            while((queue_size==0)&&(!shutdown)){
                queue_not_empty.wait(lock);
                if(wait_exit_threads>0){
                    wait_exit_threads--;
                    if(live_threads>min_threads){
                        live_threads--;

                        return;
                    }
                }
            }
        
            if(shutdown&&queue_size==0){         
                
                return;
            }
        
            task = std::move(tasks.front());
            tasks.pop();
            queue_size--;
        }
        pool_mutex.lock();
        busy_threads++;
        pool_mutex.unlock();
        task();
        pool_mutex.lock();
        busy_threads--;
        pool_mutex.unlock();
    }
    return;
    
}

thread_pool::~thread_pool(){
    shutdown = true;
    queue_not_empty.notify_all();     //唤醒所有线程
    for(std::thread &worker:workers){
        if(worker.joinable()){
            worker.join();
        }
    }
}
