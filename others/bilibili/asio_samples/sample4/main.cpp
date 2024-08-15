#include "httpserver.h"

int main() {
  try {
    asio::io_context io_context;

    // 创建HTTP服务器
    HttpServer server(io_context, 80);  // HTTP标准端口为80
    server.RegisterHandler("/hello",
                           [](std::shared_ptr<Session> session,
                              std::string& response_data, bool& keep_alive) {
                             response_data =
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Length: 13\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Connection: keep-alive\r\n"
                                 "\r\n"
                                 "Hello, world!";
                             keep_alive = true;
                           });
    server.Start();

    // 创建一个线程池
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
      threads.emplace_back([&io_context]() { io_context.run(); });
    }

    // 主线程等待中断信号
    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait(
        [&io_context](const asio::error_code&, int) { io_context.stop(); });

    // 等待所有线程完成
    for (auto& thread : threads) {
      thread.join();
    }
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
