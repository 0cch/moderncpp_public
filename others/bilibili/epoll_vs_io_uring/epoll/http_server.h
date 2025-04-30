// HTTP服务器类声明
#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <sys/epoll.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "http_connection.h"

// HTTP服务器类
class HttpServer {
 public:
  // 回调函数类型定义
  using RequestCallback = HttpConnection::RequestCallback;

  HttpServer(const std::string& ip, int port);
  ~HttpServer();

  // 启动服务器
  bool Start();

  // 停止服务器
  void Stop();

  // 运行事件循环
  void EventLoop();

  // 添加事件
  void AddEvent(int fd, int events);

  // 修改事件
  void ModifyEvent(int fd, int events);

  // 删除事件
  void RemoveEvent(int fd);

  // 移除连接
  void RemoveConnection(int fd);

  // 设置请求回调函数
  void SetRequestCallback(const RequestCallback& cb);

 private:
  static const int kMaxEvents = 1024;  // 最大事件数

  // 处理新连接
  void HandleNewConnection();

  // 处理读事件
  void HandleRead(int fd);

  // 处理写事件
  void HandleWrite(int fd);

  std::string ip_;                         // 服务器IP地址
  int port_;                               // 服务器端口
  int listen_fd_;                          // 监听套接字
  int epoll_fd_;                           // epoll文件描述符
  bool running_;                           // 服务器运行状态
  struct epoll_event events_[kMaxEvents];  // epoll事件数组
  std::map<int, std::unique_ptr<HttpConnection>> connections_;  // 连接映射表
  RequestCallback request_callback_;                            // 请求回调函数
};

#endif  // HTTP_SERVER_H_