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

void HttpConnection::OnReadData(const char* data, int bytes_read) {
  // 将数据复制到读缓冲区
  if (bytes_read > 0 && read_index_ + bytes_read <= kBufferSize) {
    memcpy(read_buffer_ + read_index_, data, bytes_read);
    read_index_ += bytes_read;

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

      // 提交写请求
      server_->SubmitWrite(sockfd_, write_buffer_.c_str(),
                           write_buffer_.size());

      // 重置请求，准备处理下一个请求
      request_.Reset();
      read_index_ = 0;
    }
  }
}

void HttpConnection::OnWriteComplete(int bytes_written) {
  if (bytes_written > 0) {
    write_index_ += bytes_written;

    // 检查是否发送完毕
    if (write_index_ < write_buffer_.size()) {
      // 继续发送剩余数据
      server_->SubmitWrite(sockfd_, write_buffer_.c_str() + write_index_,
                           write_buffer_.size() - write_index_);
    } else {
      // 发送完毕，重置写缓冲区
      write_buffer_.clear();
      write_index_ = 0;

      // 提交新的读请求
      server_->SubmitRead(sockfd_);
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

const char* HttpConnection::GetWriteBuffer() const {
  return write_buffer_.c_str();
}

size_t HttpConnection::GetWriteBufferSize() const {
  return write_buffer_.size();
}