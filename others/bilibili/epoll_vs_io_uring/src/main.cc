// 主程序入口
#include <signal.h>
#include <iostream>
#include <string>

#include "http_request.h"
#include "http_response.h"
#include "http_server.h"

// 全局服务器指针，用于信号处理
HttpServer* g_server = nullptr;

// 信号处理函数
void SignalHandler(int sig) {
  if (g_server) {
    g_server->Stop();
  }
}

// 请求处理回调函数
void HandleRequest(const HttpRequest& request, HttpResponse* response) {
  std::cout << "收到请求: " << request.GetPath() << std::endl;

  // 根据请求路径提供不同的响应
  if (request.GetPath() == "/" || request.GetPath() == "/index.html") {
    response->SetStatusCode(HttpStatusCode::k200Ok);
    response->SetHeader("Content-Type", "text/html");
    response->SetBody(
        "<html><body><h1>欢迎访问 HTTP Epoll 服务器</h1></body></html>");
  } else if (request.GetPath() == "/hello") {
    response->SetStatusCode(HttpStatusCode::k200Ok);
    response->SetHeader("Content-Type", "text/plain");
    response->SetBody("Hello, World!");
  } else if (request.GetPath() == "/info") {
    response->SetStatusCode(HttpStatusCode::k200Ok);
    response->SetHeader("Content-Type", "application/json");
    response->SetBody(
        "{\"server\": \"HTTP Epoll Server\", \"version\": \"1.0\"}");
  } else {
    // 404 未找到
    response->SetStatusCode(HttpStatusCode::k404NotFound);
    response->SetHeader("Content-Type", "text/html");
    response->SetBody(
        "<html><body><h1>404 Not "
        "Found</h1><p>请求的资源不存在</p></body></html>");
  }
}

int main(int argc, char* argv[]) {
  // 设置信号处理
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  // 默认端口
  int port = 8080;

  // 解析命令行参数
  if (argc > 1) {
    try {
      port = std::stoi(argv[1]);
    } catch (const std::exception& e) {
      std::cerr << "端口参数无效: " << e.what() << std::endl;
      return 1;
    }
  }

  std::cout << "启动 HTTP 服务器，监听端口: " << port << std::endl;

  // 创建并启动服务器
  HttpServer server("127.0.0.1", port);
  g_server = &server;

  // 设置请求处理回调
  server.SetRequestCallback(HandleRequest);

  // 启动服务器
  if (!server.Start()) {
    std::cerr << "服务器启动失败" << std::endl;
    return 1;
  }

  // 服务器主循环
  server.EventLoop();

  std::cout << "服务器已关闭" << std::endl;
  return 0;
}