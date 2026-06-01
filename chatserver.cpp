
#include"chatserver.hpp"
#include"threadpool.hpp"

chatserver::chatserver():worker_pool(4,8,20){
    listener_thread = std::thread(&chatserver::listener_start,this,std::ref(worker_pool));
    manager_thread = std::thread(&chatserver::manager_routine,this,std::ref(worker_pool));


}

//初始化监听线程
void chatserver::listener_init(){
    this->listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd<0){
        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
        exit(1);
    }
    //设置端口复用
    int opt = 1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //绑定地址端口
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_POST);
    inet_pton(AF_INET,SERVER_IP,&serv_addr.sin_addr);
    
    if(bind(listen_fd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
        exit(1);
    }
    //开始监听
    if(listen(listen_fd,10)<0){
        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
    }
}
   
//启动监听线程
void chatserver::listener_routine(thread_pool& thread_pool){
    int epoll_fd = epoll_create1(0);
    if(epoll_fd<0){
        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
    }
    epoll_event event{};
    event.data.fd = listen_fd;
    event.events = EPOLLIN;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&event)<0){
        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
    }
    std::vector<int> client_fds; //保存客户端连接的文件描述符
    while(true){
        epoll_event events[10];
        int n = epoll_wait(epoll_fd,events,10,-1);
        if(n<0){
            std::cerr<<"错误: "<<strerror(errno)<<std::endl;
            continue;
        }
        for(int i=0;i<n;++i){
            if(events[i].data.fd == listen_fd){
                //接受新连接
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(listen_fd,(sockaddr*)&client_addr,&client_len);
                if(client_fd<0){
                    std::cerr<<"错误: "<<strerror(errno)<<std::endl;
                    continue;
                }
                client_fds.push_back(client_fd);
                //将新连接添加到epoll中
                event.data.fd = client_fd;
                event.events = EPOLLIN;
                if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&event)<0){
                    std::cerr<<"错误: "<<strerror(errno)<<std::endl;
                    continue;
                }
            }
            else{
                //处理客户端请求
                int client_fd = events[i].data.fd;
                    char buffer[BUF_SIZE];
                    ssize_t ret = recv(client_fd,buffer,BUF_SIZE,0);
                    if(ret>0){
                        //将消息添加到线程池任务队列
                        std::string msg(buffer,ret);
                        thread_pool.add_task([client_fd,msg,client_fds](){
                            for (size_t i = 0; i < client_fds.size(); i++)
                            {   if(client_fds[i] != client_fd) { //不发送给自己
                                send(client_fds[i],msg.c_str(),msg.size(),MSG_NOSIGNAL);
                            }
                        }
                            
                        });
                    }
                    else if(ret==0){
                        //客户端关闭连接
                        close(client_fd);
                        client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
                        continue;
                    }
                    else{
                        std::cerr<<"错误: "<<strerror(errno)<<std::endl;
                        client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
                        close(client_fd);
                    }
            }
        }
    }
}

//初始化管理线程
void chatserver::listener_start(thread_pool& thread_pool){
    listener_init();
    listener_routine(thread_pool);
}

//运行管理线程
void chatserver::manager_routine(thread_pool& thread_pool){
    while (!shutdown)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        int live_threads, busy_threads, queue_size;
        //这里可以添加一些管理逻辑，比如调整线程池大小等
        {std::unique_lock<std::mutex> lock(thread_pool.pool_mutex);
            live_threads = thread_pool.live_threads;
            busy_threads = thread_pool.busy_threads;
            queue_size = thread_pool.queue_size;
        }
        if(queue_size>thread_pool.queue_max_size/2 && live_threads<thread_pool.max_threads){
            //增加线程
            int add_count = std::min(thread_pool.max_threads - live_threads, thread_pool.queue_max_size - queue_size);
            for(int i=0;i<add_count;++i){   
                thread_pool.workers.emplace_back(&thread_pool::thread_worker,&thread_pool); //创建新线程
                thread_pool.live_threads++;  
            }
        }
                else if(busy_threads*2<live_threads && live_threads>thread_pool.min_threads){
            //减少线程   
            thread_pool.wait_exit_threads = std::min(live_threads - thread_pool.min_threads, live_threads - busy_threads*2);
            thread_pool.queue_not_empty.notify_all();
        }
    }
    
 
}

chatserver::~chatserver(){
    worker_pool.shutdown = true;
    worker_pool.queue_not_empty.notify_all();
    if(listener_thread.joinable())
        listener_thread.join();
    if(manager_thread.joinable())
        manager_thread.join();
}
