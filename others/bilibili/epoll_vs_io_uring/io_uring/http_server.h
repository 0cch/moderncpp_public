// HTTP服务器类声明
#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <liburing.h>
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

  // 提交接受连接请求
  void SubmitAccept();

  // 提交读请求
  void SubmitRead(int fd);

  // 提交写请求
  void SubmitWrite(int fd, const void* buf, size_t len);

  // 移除连接
  void RemoveConnection(int fd);

  // 设置请求回调函数
  void SetRequestCallback(const RequestCallback& cb);

 private:
  static const int kQueueDepth = 1024;  // io_uring队列深度
  static const int kReadSize = 8192;    // 读缓冲区大小

  // 处理新连接
  void HandleNewConnection(int client_fd, struct sockaddr_in* client_addr);

  // 处理读事件
  void HandleRead(int fd, const char* data, int bytes_read);

  // 处理写事件
  void HandleWrite(int fd, int bytes_written);

  std::string ip_;                         // 服务器IP地址
  int port_;                               // 服务器端口
  int listen_fd_;                          // 监听套接字
  bool running_;                           // 服务器运行状态
  struct io_uring ring_;                   // io_uring实例
  std::map<int, std::unique_ptr<HttpConnection>> connections_;  // 连接映射表
  RequestCallback request_callback_;       // 请求回调函数
};

#endif  // HTTP_SERVER_H_