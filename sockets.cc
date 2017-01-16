#include "sockets.h"

// system headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <glog/logging.h>

// libevent
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/thread.h>

#include <string>

namespace sockets{
// 最大每个数据包为100k
const int kMaxPackSize = 100 * 1024;
const int kPort = 7777;
const int kMaxConnectSize = 50*1000;
// 读取回调
static void read_cb(struct bufferevent* bev, void* arg);
// 写入回调
static void write_cb(struct bufferevent* bev, void* arg);
// 事件回调
static void event_cb(struct bufferevent* bev, short events, void* arg);
// 信号回调
static void signal_cb(evutil_socket_t, short events, void *);
// 监听回调
static void accept_cb(struct evconnlistener *, evutil_socket_t fd, struct sockaddr * sa, int socklen, void *arg);
// 接收错误回调
static void accept_error_cb(struct evconnlistener* listener, void * ctx);

// 启动服务
int StartServer(int port)
{
    int error = evthread_use_pthreads();
    
    struct rlimit rt;  
    /* 设置每个进程允许打开的最大文件数 */  
    rt.rlim_max = rt.rlim_cur = kMaxConnectSize;  
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1){  
        LOG(ERROR) << "setrlimit error";  
        return -1;
    }  
   if (error != 0){
        LOG(ERROR) << "use evthread_use_pthreads() functional error.";
   }

    LOG(INFO) << "fd size : " << FD_SETSIZE;
    struct event_base* base;
    struct evconnlistener* listener;
    struct event* signal_event;

    struct sockaddr_in sin;

    // 首先构建base
    base = event_base_new();
    if (!base){
        LOG(ERROR) << "Could not initialize livevent!";
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);


    // 创建监听器
    listener = ::evconnlistener_new_bind(base, accept_cb, (void*)base, 
           LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin, sizeof(sin));

    if (!listener){
        LOG(ERROR) << "Could not create listener!";
    }

    // 中断信号
#if 0
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);
    if (!signal_event || event_add(signal_event, NULL)){
            LOG(ERROR) << "Could not create or add a signal event!";
    }
#endif 

    // 设置listener错误回调
    evconnlistener_set_error_cb(listener, accept_error_cb);

    // loop
    event_base_dispatch(base);
    evconnlistener_free(listener);
    
    // event_free(signal_event);
    event_base_free(base);

    return 0;
}

// callback functionals
static void accept_cb(struct evconnlistener *, evutil_socket_t fd, struct sockaddr * sa, int socklen, void *arg){
    LOG(INFO) << "a new connection. fd is " << fd;
    struct event_base* base = (struct event_base*)arg;
    struct bufferevent* bev;
    int optval = 1;

    // 不使用Nagle算法，选择立即发送数据而不是等待产生更多的数据然后再一次发送
    // 保持长连接
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, sizeof(optval));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&optval, sizeof(optval));
    
    // 设置异步非阻塞模式
    evutil_make_socket_nonblocking(fd);

    // 得到一个新的连接，通过连接fd，构建一个bufferevent
    // BEV_OPT_HTREADSAFE 线程安全模式 
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    // bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    if (!bev){
        LOG(ERROR) << "constructing bufferevent error!";
        event_base_loopbreak(base);
        return;
    }

    //  truct timeval tv;
    //  tv.tv_usec = 1000;
    //  tv.tv_sec = 1;
    // bufferevent_set_timeouts(bev,&tv, &tv);

    // 设置bufferevent的回调函数
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

}

// 读取回调
// This callback is invorked when there is data to read on bev.
static void read_cb(struct bufferevent* bev, void* arg){
    evutil_socket_t fd =  bufferevent_getfd(bev);
    // LOG(INFO) << "read data from  fd is " << fd;

    char buf[256] = {0};
    uint32_t n, len;
    n = bufferevent_read(bev, (char*)&len, sizeof(uint32_t));

    if (n != sizeof(n)){
        LOG(ERROR) << "read size error " << n;
       // return;
    }

    // LOG(INFO) << "READ " << len << " : " << ntohl(len);
    std::string send_buf = "hello libevent!\n";
    bufferevent_write(bev, (char*)&*send_buf.begin(), send_buf.length());
}

// 读取回调
static void write_cb(struct bufferevent* bev, void* arg){
   //  LOG(INFO) << "invork write_cb functional.";
}

// 事件回调
static void event_cb(struct bufferevent* bev, short events, void* arg){
    evutil_socket_t fd =  bufferevent_getfd(bev);
    // LOG(INFO) << "event callback fd is " << fd;

    if (events & BEV_EVENT_ERROR) {
        LOG(ERROR) << "happen BEV_EVENT_ERROR error fd is " << fd;
    }

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        LOG(INFO) << "close events and socket. fd is " << fd; 
        bufferevent_free(bev);
    }

/*    // 读写超时 
    if (events & (BEV_EVENT_TIMEOUT | BEV_EVENT_READING)){
         bufferevent_enable(bev, EV_READ | EV_WRITE);
         LOG(ERROR) << "Read timeout error.";
    }
    if (events & (BEV_EVENT_TIMEOUT | BEV_EVENT_WRITING)){
         bufferevent_enable(bev, EV_READ | EV_WRITE);
         LOG(ERROR) << "write timeout error.";
  }
*/
}

// 信号回调
static void signal_cb(evutil_socket_t, short, void *){
    LOG(INFO) << "signal callback";
}

// 接收错误回调
static void accept_error_cb(struct evconnlistener* listener, void * ctx){
    struct event_base * base = evconnlistener_get_base(listener);
    int error = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "occur a error when listening, error is " << error;
    LOG(ERROR) << "***shutdown the listener***";

    event_base_loopexit(base, NULL); 
}



} // !namespace sockets 
