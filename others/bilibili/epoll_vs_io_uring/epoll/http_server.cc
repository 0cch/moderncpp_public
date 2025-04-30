// HTTP服务器类实现
#include "http_server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

HttpServer::HttpServer(const std::string& ip, int port)
    : ip_(ip), port_(port), listen_fd_(-1), epoll_fd_(-1), running_(false) {}

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

  // 创建epoll实例
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ < 0) {
    std::cerr << "创建epoll实例失败: " << strerror(errno) << std::endl;
    close(listen_fd_);
    return false;
  }

  // 添加监听套接字到epoll
  AddEvent(listen_fd_, EPOLLIN);

  running_ = true;
  std::cout << "HTTP服务器启动成功，监听 " << ip_ << ":" << port_ << std::endl;

  return true;
}

void HttpServer::Stop() {
  running_ = false;

  // 关闭所有连接
  connections_.clear();

  // 关闭epoll实例
  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
    epoll_fd_ = -1;
  }

  // 关闭监听套接字
  if (listen_fd_ >= 0) {
    close(listen_fd_);
    listen_fd_ = -1;
  }

  std::cout << "HTTP服务器已停止" << std::endl;
}

void HttpServer::EventLoop() {
  while (running_) {
    int num_events = epoll_wait(epoll_fd_, events_, kMaxEvents, -1);

    if (num_events < 0) {
      if (errno == EINTR) {
        continue;  // 被信号中断，继续循环
      }
      std::cerr << "epoll_wait错误: " << strerror(errno) << std::endl;
      break;
    }

    for (int i = 0; i < num_events; ++i) {
      int fd = events_[i].data.fd;

      // 处理新连接
      if (fd == listen_fd_) {
        HandleNewConnection();
        continue;
      }

      // 处理错误事件
      if (events_[i].events & (EPOLLERR | EPOLLHUP)) {
        RemoveConnection(fd);
        continue;
      }

      // 处理读事件
      if (events_[i].events & EPOLLIN) {
        HandleRead(fd);
      }

      // 处理写事件
      if (events_[i].events & EPOLLOUT) {
        HandleWrite(fd);
      }
    }
  }
}

void HttpServer::AddEvent(int fd, int events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

void HttpServer::ModifyEvent(int fd, int events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

void HttpServer::RemoveEvent(int fd) {
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void HttpServer::RemoveConnection(int fd) {
  if (connections_.find(fd) != connections_.end()) {
    RemoveEvent(fd);
    connections_.erase(fd);
  }
}

void HttpServer::HandleNewConnection() {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int client_fd =
      accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd < 0) {
    std::cerr << "接受连接失败: " << strerror(errno) << std::endl;
    return;
  }

  // 设置非阻塞模式
  int flags = fcntl(client_fd, F_GETFL, 0);
  fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

  // 添加到epoll
  AddEvent(client_fd, EPOLLIN);

  // 创建连接对象
  auto conn = std::make_unique<HttpConnection>(client_fd, this);
  conn->SetRequestCallback(request_callback_);
  connections_[client_fd] = std::move(conn);

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
  std::cout << "新连接: " << client_ip << ":" << ntohs(client_addr.sin_port)
            << std::endl;
}

void HttpServer::HandleRead(int fd) {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    it->second->OnRead();
  }
}

void HttpServer::HandleWrite(int fd) {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    it->second->OnWrite();
  }
}

void HttpServer::SetRequestCallback(const RequestCallback& cb) {
  request_callback_ = cb;

  // 为现有连接设置回调
  for (auto& conn : connections_) {
    conn.second->SetRequestCallback(cb);
  }
}