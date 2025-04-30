// HTTP服务器类实现
#include "http_server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <liburing.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

// 请求类型枚举
enum {
  ACCEPT,
  READ,
  WRITE
};

// 请求结构体
struct io_request {
  int type;
  int fd;
  char* buf;
  size_t len;
  struct sockaddr_in client_addr;
  socklen_t client_len;
};

HttpServer::HttpServer(const std::string& ip, int port)
    : ip_(ip), port_(port), listen_fd_(-1), running_(false) {}

HttpServer::~HttpServer() {
  Stop();
}

bool HttpServer::Start() {
  // 创建监听套接字
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    std::cerr << "创建套接字失败: " << strerror(errno) << std::endl;
    return false;
  }

  // 设置套接字选项
  int opt = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "设置套接字选项失败: " << strerror(errno) << std::endl;
    close(listen_fd_);
    return false;
  }

  // 设置非阻塞模式
  int flags = fcntl(listen_fd_, F_GETFL, 0);
  fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);

  // 绑定地址
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

  if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "绑定地址失败: " << strerror(errno) << std::endl;
    close(listen_fd_);
    return false;
  }

  // 开始监听
  if (listen(listen_fd_, SOMAXCONN) < 0) {
    std::cerr << "监听失败: " << strerror(errno) << std::endl;
    close(listen_fd_);
    return false;
  }

  // 初始化io_uring
  struct io_uring_params params;
  memset(&params, 0, sizeof(params));
  
  if (io_uring_queue_init_params(kQueueDepth, &ring_, &params) < 0) {
    std::cerr << "创建io_uring实例失败: " << strerror(errno) << std::endl;
    close(listen_fd_);
    return false;
  }

  running_ = true;
  std::cout << "HTTP服务器启动成功，监听 " << ip_ << ":" << port_ << std::endl;

  // 提交第一个接受连接请求
  SubmitAccept();

  return true;
}

void HttpServer::Stop() {
  running_ = false;

  // 关闭所有连接
  connections_.clear();

  // 关闭io_uring实例
  io_uring_queue_exit(&ring_);

  // 关闭监听套接字
  if (listen_fd_ >= 0) {
    close(listen_fd_);
    listen_fd_ = -1;
  }

  std::cout << "HTTP服务器已停止" << std::endl;
}

void HttpServer::SubmitAccept() {
  struct io_request* req = new io_request;
  req->type = ACCEPT;
  req->fd = listen_fd_;
  req->client_len = sizeof(req->client_addr);

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  io_uring_prep_accept(sqe, listen_fd_, (struct sockaddr*)&req->client_addr, 
                      &req->client_len, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring_);
}

void HttpServer::SubmitRead(int fd) {
  struct io_request* req = new io_request;
  req->type = READ;
  req->fd = fd;
  req->buf = new char[kReadSize];
  req->len = kReadSize;

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  io_uring_prep_read(sqe, fd, req->buf, req->len, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring_);
}

void HttpServer::SubmitWrite(int fd, const void* buf, size_t len) {
  struct io_request* req = new io_request;
  req->type = WRITE;
  req->fd = fd;
  req->buf = new char[len];
  memcpy(req->buf, buf, len);
  req->len = len;

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  io_uring_prep_write(sqe, fd, req->buf, len, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring_);
}

void HttpServer::EventLoop() {
  while (running_) {
    struct io_uring_cqe* cqe;
    int ret = io_uring_wait_cqe(&ring_, &cqe);
    
    if (ret < 0) {
      if (errno == EINTR) {
        continue;  // 被信号中断，继续循环
      }
      std::cerr << "io_uring_wait_cqe错误: " << strerror(errno) << std::endl;
      break;
    }

    struct io_request* req = (struct io_request*)io_uring_cqe_get_data(cqe);
    int res = cqe->res;
    
    if (res < 0) {
      // 处理错误
      if (req->type == ACCEPT) {
        std::cerr << "接受连接失败: " << strerror(-res) << std::endl;
        // 继续接受新连接
        SubmitAccept();
      } else if (req->type == READ || req->type == WRITE) {
        std::cerr << "I/O操作失败: " << strerror(-res) << " fd=" << req->fd << std::endl;
        RemoveConnection(req->fd);
      }
    } else {
      // 处理成功的I/O操作
      switch (req->type) {
        case ACCEPT: {
          int client_fd = res;
          HandleNewConnection(client_fd, &req->client_addr);
          // 继续接受新连接
          SubmitAccept();
          break;
        }
        case READ: {
          if (res == 0) {
            // 连接关闭
            RemoveConnection(req->fd);
          } else {
            // 处理读取的数据
            HandleRead(req->fd, req->buf, res);
          }
          break;
        }
        case WRITE: {
          // 处理写完成
          HandleWrite(req->fd, res);
          break;
        }
      }
    }

    // 释放请求资源
    if (req->type == READ || req->type == WRITE) {
      delete[] req->buf;
    }
    delete req;

    // 标记完成队列条目为已处理
    io_uring_cqe_seen(&ring_, cqe);
  }
}

void HttpServer::HandleNewConnection(int client_fd, struct sockaddr_in* client_addr) {
  // 设置非阻塞模式
  int flags = fcntl(client_fd, F_GETFL, 0);
  fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

  // 创建连接对象
  auto conn = std::make_unique<HttpConnection>(client_fd, this);
  conn->SetRequestCallback(request_callback_);
  connections_[client_fd] = std::move(conn);

  // 提交读请求
  SubmitRead(client_fd);

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
  std::cout << "新连接: " << client_ip << ":" << ntohs(client_addr->sin_port)
            << std::endl;
}

void HttpServer::HandleRead(int fd, const char* data, int bytes_read) {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    // 需要修改HttpConnection类以适应新的数据传递方式
    it->second->OnReadData(data, bytes_read);
    
    // 继续提交读请求
    SubmitRead(fd);
  }
}

void HttpServer::HandleWrite(int fd, int bytes_written) {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    // 需要修改HttpConnection类以适应新的数据传递方式
    it->second->OnWriteComplete(bytes_written);
  }
}

void HttpServer::RemoveConnection(int fd) {
  if (connections_.find(fd) != connections_.end()) {
    connections_.erase(fd);
    close(fd);
  }
}

void HttpServer::SetRequestCallback(const RequestCallback& cb) {
  request_callback_ = cb;

  // 为现有连接设置回调
  for (auto& conn : connections_) {
    conn.second->SetRequestCallback(cb);
  }
}