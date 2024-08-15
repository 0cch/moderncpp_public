/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include "httpserver2.h"

HttpServer::HttpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.listen();
}

void HttpServer::Start() { Accept(); }

void HttpServer::Accept() {
  auto session = std::make_shared<Session>(io_context_);
  acceptor_.async_accept(session->socket(),
                         [this, session](const asio::error_code& ec) {
                           if (!ec) {
                             HandleAccept(ec, session);
                           } else {
                             Accept();
                           }
                         });
}

void HttpServer::HandleAccept(const asio::error_code& ec,
                              std::shared_ptr<Session> session) {
  if (!ec) {
    auto buffer = std::make_shared<asio::streambuf>();
    StartTimer(session);
    asio::async_read_until(
        session->socket(), *buffer, "\r\n\r\n",
        [this, session, buffer](const asio::error_code& ec, std::size_t) {
          CancelTimer(session);
          HandleRequestHeaders(ec, session, buffer);
        });
  }
  Accept();
}

void HttpServer::HandleRequestHeaders(const asio::error_code& ec,
                                      std::shared_ptr<Session> session,
                                      std::shared_ptr<asio::streambuf> buffer) {
  if (!ec) {
    std::istream request_stream(buffer.get());
    std::string request_line;
    std::getline(request_stream, request_line);

    std::string header;
    std::size_t content_length = 0;
    bool keep_alive = true;
    while (std::getline(request_stream, header) && header != "\r") {
      if (header.find("Content-Length:") == 0) {
        content_length = std::stoul(header.substr(15));
      }
      if (header.find("Connection:") == 0) {
        std::string connection = header.substr(11);
        if (connection.find("close") != std::string::npos) {
          keep_alive = false;
        }
      }
    }

    if (content_length > 0) {
      StartTimer(session);
      asio::async_read(
          session->socket(), *buffer, asio::transfer_exactly(content_length),
          [this, session, buffer, content_length, keep_alive](
              const asio::error_code& ec, std::size_t) {
            CancelTimer(session);
            HandleRequestBody(ec, session, buffer, content_length, keep_alive);
          });
    } else {
      HandleRequestBody(ec, session, buffer, 0, keep_alive);
    }
  } else {
    std::cout << "Handle request headers error: " << ec.message() << "\n";
  }
}

void HttpServer::HandleRequestBody(const asio::error_code& ec,
                                   std::shared_ptr<Session> session,
                                   std::shared_ptr<asio::streambuf> buffer,
                                   std::size_t content_length,
                                   bool keep_alive) {
  if (!ec) {
    auto response = std::make_shared<asio::streambuf>();
    std::ostream response_stream(response.get());

    // 生成HTTP响应
    response_stream << "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 13\r\n"
                       "Content-Type: text/plain\r\n"
                       "Connection: keep-alive\r\n"
                       "\r\n"
                       "Hello, world!";

    SendResponse(ec, session, response, keep_alive);
  } else {
    std::cerr << "Request body read failed: " << ec.message() << "\n";
  }
}

void HttpServer::SendResponse(const asio::error_code& ec,
                              std::shared_ptr<Session> session,
                              std::shared_ptr<asio::streambuf> response,
                              bool keep_alive) {
  if (!ec) {
    StartTimer(session);
    asio::async_write(
        session->socket(), *response,
        [this, session, response, keep_alive](const asio::error_code& ec,
                                              std::size_t) {
          CancelTimer(session);
          if (!ec) {
            if (keep_alive) {
              // 继续读取新的请求
              auto buffer = std::make_shared<asio::streambuf>();
              asio::async_read_until(
                  session->socket(), *buffer, "\r\n\r\n",
                  [this, session, buffer](const asio::error_code& ec,
                                          std::size_t) {
                    HandleRequestHeaders(ec, session, buffer);
                  });
            } else {
              // 关闭连接
              session->socket().shutdown(tcp::socket::shutdown_both);
              session->socket().close();
            }
          } else {
            std::cerr << "Write failed: " << ec.message() << "\n";
          }
        });
  } else {
    std::cerr << "Response send failed: " << ec.message() << "\n";
  }
}

void HttpServer::StartTimer(std::shared_ptr<Session> session) {
  session->timer().expires_after(std::chrono::seconds(30));
  session->timer().async_wait([this, session](const asio::error_code& ec) {
    if (!ec) {
      session->socket().cancel();
      std::cerr << "Timeout, closing socket.\n";
      session->socket().shutdown(tcp::socket::shutdown_both);
      session->socket().close();
    }
  });
}

void HttpServer::CancelTimer(std::shared_ptr<Session> session) {
  session->timer().cancel();
}

int main() {
  try {
    asio::io_context io_context;

    // 创建HTTP服务器
    HttpServer server(io_context, 80);  // HTTP标准端口为80
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
