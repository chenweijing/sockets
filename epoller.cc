// Author : chenwj
// Time   : 2017/1/16
// Copyright :All rights reserved by chenwj @copyright 2017 ~ 2018.
// 
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
// 
// Decription: base linux epoll network interface. Use EPOLLET and EPOLLLT
// style. This program surpot 100*10000 connect sockets.
// At the Begin, app will new 2G memory that being provied for server.
// Four(4) or more worker thread will start to deal with comming data.
// For every thread, had a index queue, to comsumer the data by produer/comsumer model.
// 
////////////////////////////////////////////////////////////////////////////////////////

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

/*
 * max connections
 * s can be changed in /etc/security/limits.conf - it's the nofile param.
 * However, if you're closing your sockets correctly, you shouldn't receive this unless you're opening a lot of simulataneous connections. 
 * It sounds like something is preventing your sockets from being closed appropriately. I would verify that they are being handled properly.
*/


namespace net{
// per package max size is 100k
const int kMaxPackSize = 100 * 1024;
const int kPort = 7777;
const int kMaxConnectSize = 10*10000;
const int kMaxEpollEvents = 50*1000;
#define MAX_EVENTS 10

//
// socket options
//
static void setNonBlocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

static void setTcpNoDelay(int sockfd, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof optval));
}

static void setReuseAddr(int sockfd, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
}

static void setReusePort(int sockfd, bool on)
{
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on){
    LOG(ERROR)<< "SO_REUSEPORT failed.";
  }
}

static void setKeepAlive(int sockfd, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
}
static void setSocketOpsAll(int sockfd, bool on)
{
    setNonBlocking(sockfd);
    setTcpNoDelay(sockfd, on);
    setKeepAlive(sockfd, on);
    setReuseAddr(sockfd, on);
    setReusePort(sockfd, on);
}

//  socket listen 
int socketListen(int port)
{
    int sockfd;
    // set max connections.
    struct rlimit rt;  
    rt.rlim_max = rt.rlim_cur = kMaxConnectSize;  
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1){  
        LOG(ERROR) << "setrlimit error";  
        return -1;
    }  
  
    // create a listen socket
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0))<0){  
        LOG(ERROR) << "create a socket.";
        return -1;
    }  

    // listen address
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET; 

    if (bind(sockfd,(struct sockaddr *)&addr,sizeof(addr))<0){  
        LOG(ERROR) << "bind failed.";
        return -1;
    }  
    if (listen(sockfd, 5)<0){  
        LOG(ERROR) << "listen failed. port : " << port;
        return -1;
    }  
    
    bool on = true;
    setTcpNoDelay(sockfd, on);
    setKeepAlive(sockfd, on);
    setReuseAddr(sockfd, on);
    setReusePort(sockfd, on);

    return sockfd;
}

// epoll loop
void eventLoop(int listen_sock)
{
    int64_t conn_num = 0;
    struct epoll_event ev, events[MAX_EVENTS];
    int n, conn_sock, nfds, epollfd;
    struct sockaddr_in local;
    socklen_t socklen;
    /* Set up listening socket, 'listen_sock' (socket(),
       bind(), listen()) */

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        LOG(ERROR) << "epoll_create failed.";
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    // ev.events = EPOLLET;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        LOG(ERROR) << "epoll_ctl: listen_sock";
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            LOG(ERROR) << "epoll_pwait";
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) {
            // a new connection come
            if (events[n].data.fd == listen_sock) {
                conn_sock = ::accept(listen_sock, (struct sockaddr *) &local, &socklen);
                if (conn_sock == -1) {
                    LOG(ERROR) << "accept";
                    exit(EXIT_FAILURE);
                }
                setSocketOpsAll(conn_sock, true);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    LOG(ERROR)<< "epoll_ctl: conn_sock";
                    exit(EXIT_FAILURE);
                }
                ++conn_num; 
                LOG(INFO)<< "A new connection come. conns : " << conn_num; 
            } else {
               // do_use_fd(events[n].data.fd);
               if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP)){
                   LOG(ERROR) << "epoll error or epoll hup " << events[n].data.fd << " close.";
                   ::close(events[n].data.fd);
                   epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                   --conn_num;
                   continue;
               }

               if (events[n].events & EPOLL_CTL_DEL ) {
                   LOG(INFO) << events[n].data.fd   << " close.";
                   ::close(events[n].data.fd);
                   epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                   --conn_num;
                   continue;
               }

               if (events[n].events & EPOLLIN){
                    uint32_t len = 0;
                    int datalen = ::recv(events[n].data.fd, (char*)&len, sizeof(u_int32_t),0); 
                    if (datalen <= 0){
                        ::close(events[n].data.fd);
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                        LOG(INFO) << events[n].data.fd << " close.";
                        --conn_num;
                        continue;
                    }
                    // LOG(INFO) << "READ " << ntohl(len);
                    std::string sdata = "hello, epoll!";
                    ::send(events[n].data.fd, (char*)&*sdata.begin(), sdata.length(), 0);
               }
            }
        }
    }
}

// start server
void start(int port)
{
   int sockfd = socketListen(port);
   setReusePort(sockfd, true);
   eventLoop(sockfd);
}


} // !namespace sockets 
