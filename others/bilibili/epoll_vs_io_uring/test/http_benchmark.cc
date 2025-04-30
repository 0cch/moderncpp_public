#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// 原子计数器，用于统计成功的请求数
std::atomic<int> success_count(0);
std::atomic<int> fail_count(0);

// 发送HTTP请求并接收响应
bool SendHttpRequest(const std::string& ip, int port, const std::string& path) {
  // 创建套接字
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    return false;
  }

  // 设置服务器地址
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

  // 连接服务器
  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    close(sockfd);
    return false;
  }

  // 构造HTTP请求
  std::string request = "GET " + path + " HTTP/1.1\r\n";
  request += "Host: " + ip + ":" + std::to_string(port) + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";

  // 发送请求
  if (write(sockfd, request.c_str(), request.size()) !=
      static_cast<ssize_t>(request.size())) {
    close(sockfd);
    return false;
  }

  // 接收响应
  char buffer[4096];
  ssize_t bytes_read = read(sockfd, buffer, sizeof(buffer) - 1);

  // 关闭套接字
  close(sockfd);

  // 检查是否成功接收到响应
  if (bytes_read <= 0) {
    return false;
  }

  // 确保响应以null结尾
  buffer[bytes_read] = '\0';

  // 检查是否是HTTP响应
  return strstr(buffer, "HTTP/1.1") != nullptr;
}

// 工作线程函数
void WorkerThread(const std::string& ip,
                  int port,
                  const std::string& path,
                  int requests_per_thread) {
  for (int i = 0; i < requests_per_thread; ++i) {
    if (SendHttpRequest(ip, port, path)) {
      success_count++;
    } else {
      fail_count++;
    }
  }
}

int main(int argc, char* argv[]) {
  // 默认参数
  std::string ip = "127.0.0.1";
  int port = 8080;
  std::string path = "/";
  int num_threads = 10;
  int requests_per_thread = 100;

  // 解析命令行参数
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 < argc) {
      std::string arg = argv[i];
      if (arg == "--ip" || arg == "-i") {
        ip = argv[i + 1];
      } else if (arg == "--port" || arg == "-p") {
        port = std::stoi(argv[i + 1]);
      } else if (arg == "--path") {
        path = argv[i + 1];
      } else if (arg == "--threads" || arg == "-t") {
        num_threads = std::stoi(argv[i + 1]);
      } else if (arg == "--requests" || arg == "-r") {
        requests_per_thread = std::stoi(argv[i + 1]);
      }
    }
  }

  std::cout << "HTTP压力测试开始" << std::endl;
  std::cout << "目标服务器: " << ip << ":" << port << path << std::endl;
  std::cout << "线程数: " << num_threads << std::endl;
  std::cout << "每线程请求数: " << requests_per_thread << std::endl;
  std::cout << "总请求数: " << num_threads * requests_per_thread << std::endl;

  // 创建线程
  std::vector<std::thread> threads;

  // 记录开始时间
  auto start_time = std::chrono::high_resolution_clock::now();

  // 启动工作线程
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(WorkerThread, ip, port, path, requests_per_thread);
  }

  // 等待所有线程完成
  for (auto& thread : threads) {
    thread.join();
  }

  // 记录结束时间
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  // 计算统计信息
  int total_requests = num_threads * requests_per_thread;
  int total_success = success_count.load();
  int total_fail = fail_count.load();
  double success_rate =
      static_cast<double>(total_success) / total_requests * 100.0;
  double requests_per_second =
      static_cast<double>(total_success) / (duration.count() / 1000.0);

  // 输出结果
  std::cout << "\n测试结果:" << std::endl;
  std::cout << "总耗时: " << duration.count() << " 毫秒" << std::endl;
  std::cout << "成功请求: " << total_success << std::endl;
  std::cout << "失败请求: " << total_fail << std::endl;
  std::cout << "成功率: " << success_rate << "%" << std::endl;
  std::cout << "每秒请求数 (QPS): " << requests_per_second << std::endl;

  return 0;
}