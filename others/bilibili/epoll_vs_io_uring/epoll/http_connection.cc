// HTTP连接类实现
#include "http_connection.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "http_server.h"

HttpConnection::HttpConnection(int sockfd, HttpServer* server)
    : sockfd_(sockfd), server_(server), read_index_(0), write_index_(0) {}

HttpConnection::~HttpConnection() {
  if (sockfd_ >= 0) {
    close(sockfd_);
    sockfd_ = -1;
  }
}

void HttpConnection::OnRead() {
  // 读取数据
  ssize_t n =
      read(sockfd_, read_buffer_ + read_index_, kBufferSize - read_index_);

  if (n > 0) {
    read_index_ += n;

    // 解析HTTP请求
    if (request_.Parse(read_buffer_, read_index_)) {
      // 请求解析完成，生成响应
      HttpResponse response;

      // 调用回调函数处理请求
      if (request_callback_) {
        request_callback_(request_, &response);
      } else {
        // 默认响应
        response.SetStatusCode(HttpStatusCode::k404NotFound);
        response.SetBody("<html><body><h1>404 Not Found</h1></body></html>");
        response.SetHeader("Content-Type", "text/html");
      }

      // 生成响应字符串
      write_buffer_ = response.ToString();
      write_index_ = 0;

      // 触发写事件
      server_->ModifyEvent(sockfd_, EPOLLOUT);

      // 重置请求，准备处理下一个请求
      request_.Reset();
      read_index_ = 0;
    }
  } else if (n == 0) {
    // 对端关闭连接
    Close();
  } else {
    // 读取错误
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      Close();
    }
  }
}

void HttpConnection::OnWrite() {
  // 发送数据
  ssize_t n = write(sockfd_, write_buffer_.c_str() + write_index_,
                    write_buffer_.size() - write_index_);

  if (n > 0) {
    write_index_ += n;

    // 检查是否发送完毕
    if (write_index_ >= write_buffer_.size()) {
      // 发送完毕，重新注册读事件
      server_->ModifyEvent(sockfd_, EPOLLIN);
      write_buffer_.clear();
      write_index_ = 0;
    }
  } else {
    // 写入错误
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      Close();
    }
  }
}

void HttpConnection::Close() {
  if (sockfd_ >= 0) {
    server_->RemoveConnection(sockfd_);
  }
}

int HttpConnection::GetFd() const {
  return sockfd_;
}

void HttpConnection::SetRequestCallback(const RequestCallback& cb) {
  request_callback_ = cb;
}