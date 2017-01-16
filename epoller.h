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


#ifndef _EPOLLER_H_
#define _EPOLLER_H_

namespace net
{
// start a server.
void start(int port);

} // !namespace sockets

#endif
