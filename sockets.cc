#include "sockets.h"

// system headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <glog/logging.h>

namespace sockets{
// 服务器监听
// \port 监听端口

int start(bool is_block, char * port){
    // create socket
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family =   AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0){
        LOG(ERROR) << "getaddrinof: " << gai_strerror(s);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next){
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0){
            // we managed to bind successfully!
            break; 
        }

        close(sfd);
    }
    
    if (rp == NULL){
        LOG(ERROR) << "Could not bind " << port;
        return -1;
    }

    freeaddrinfo(result);
    

    // set noblocking
    int flags;
    flags = fcntl(sfd, F_GETFL,0);
    if (flags == -1){
        LOG(ERROR) << "error fcntl read";
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (flags == -1){
        LOG(ERROR) << "error fcntl setting";
    }

    // listen
    s = ::listen(sfd, SOMAXCONN);
    if (s == -1){
        LOG(ERROR) << "listen error " << port;
    }

    int efd = epoll_create1(0);

    return sfd;
}












































} // !namespace sockets 
