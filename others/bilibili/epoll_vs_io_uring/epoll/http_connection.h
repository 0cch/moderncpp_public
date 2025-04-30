// HTTP连接类声明
#ifndef HTTP_CONNECTION_H_
#define HTTP_CONNECTION_H_

#include <functional>
#include <memory>
#include <string>

#include "http_request.h"
#include "http_response.h"

// 前向声明
class HttpServer;

// HTTP连接类
class HttpConnection {
 public:
  // 回调函数类型定义
  using RequestCallback =
      std::function<void(const HttpRequest&, HttpResponse*)>;

  HttpConnection(int sockfd, HttpServer* server);
  ~HttpConnection();

  // 处理读事件
  void OnRead();

  // 处理写事件
  void OnWrite();

  // 关闭连接
  void Close();

  // 获取套接字描述符
  int GetFd() const;

  // 设置请求回调函数
  void SetRequestCallback(const RequestCallback& cb);

 private:
  static const size_t kBufferSize = 4096;  // 缓冲区大小

  int sockfd_;                        // 套接字描述符
  HttpServer* server_;                // 所属的HTTP服务器
  char read_buffer_[kBufferSize];     // 读缓冲区
  size_t read_index_;                 // 读缓冲区中已读取的数据长度
  std::string write_buffer_;          // 写缓冲区
  size_t write_index_;                // 写缓冲区中已写入的数据长度
  HttpRequest request_;               // HTTP请求
  RequestCallback request_callback_;  // 请求回调函数
};

#endif  // HTTP_CONNECTION_H_