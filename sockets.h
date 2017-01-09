#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include <memory>
#include <functional>

using namespace std;

// 作者：chenwj
// 基于epoll网络通信库
// 采用c风格形式，不用c++风格，代码简约
// 
// 主要分四个部分：
// 1，字节序转换
// 2，填充数据到buffer
// 3，NETWORK通信
// 4，回调类处理
// 
// 为什么使用原始的epoll
// 理由：代码简单，明白，对于几十万的并发的量可以轻松处理
// 依赖系统级函数，基本做到零依赖和安装
namespace sockets{
// 接受一个新的套接字回调
typedef std::function<void (int fd)> accept_callback_t;
// 读取数据回调函数
typedef std::function<void (int fd, char * buff, int size)> read_callback_t;
// 服务器启动
// \is_block 是否阻塞
int start(bool is_block, char * port);
// 客户端连接
bool connect(const std::string & server, int32_t port);
// 设置新的链接回调
void setAcceptCallback(const std::shared_ptr<accept_callback_t> & accept_callback);
// 设置读取数据回调
void setReadCallback(const std::shared_ptr<read_callback_t> & read_callback);
}
#endif
