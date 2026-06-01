#include"chatserver.hpp"

int main(){
    chatserver chat_server;
    // 构造函数已启动 listener_thread 和 manager_thread，主线程等待即可
    chat_server.listener_thread.join();
    return 0;
}