/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include "httpserver.h"

HttpServer::HttpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.listen();
}

void HttpServer::Start() { Accept(); }

void HttpServer::Accept() {
  auto socket = std::make_shared<tcp::socket>(io_context_);
  acceptor_.async_accept(*socket, [this, socket](const asio::error_code& ec) {
    HandleAccept(ec, socket);
  });
}

void HttpServer::HandleAccept(const asio::error_code& ec,
                              std::shared_ptr<tcp::socket> socket) {
  if (!ec) {
    auto buffer = std::make_shared<asio::streambuf>();
    asio::async_read_until(
        *socket, *buffer, "\r\n\r\n",
        [this, socket, buffer](const asio::error_code& ec, std::size_t) {
          HandleRequestHeaders(ec, socket, buffer);
        });
  }

  Accept();
}

void HttpServer::HandleRequestHeaders(const asio::error_code& ec,
                                      std::shared_ptr<tcp::socket> socket,
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
      asio::async_read(*socket, *buffer, asio::transfer_exactly(content_length),
                       [this, socket, buffer, content_length, keep_alive](
                           const asio::error_code& ec, std::size_t) {
                         HandleRequestBody(ec, socket, buffer, content_length,
                                           keep_alive);
                       });
    } else {
      HandleRequestBody(ec, socket, buffer, 0, keep_alive);
    }
  } else {
    std::cout << "Handle request headers error: " << ec.message() << "\n";
  }
}

void HttpServer::HandleRequestBody(const asio::error_code& ec,
                                   std::shared_ptr<tcp::socket> socket,
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

    SendResponse(ec, socket, response, keep_alive);
  } else {
    std::cerr << "Request body read failed: " << ec.message() << "\n";
  }
}

void HttpServer::SendResponse(const asio::error_code& ec,
                              std::shared_ptr<tcp::socket> socket,
                              std::shared_ptr<asio::streambuf> response,
                              bool keep_alive) {
  if (!ec) {
    asio::async_write(*socket, *response,
                      [this, socket, response, keep_alive](
                          const asio::error_code& ec, std::size_t) {
                        if (!ec) {
                          if (keep_alive) {
                            // 继续读取新的请求
                            auto buffer = std::make_shared<asio::streambuf>();
                            asio::async_read_until(
                                *socket, *buffer, "\r\n\r\n",
                                [this, socket, buffer](
                                    const asio::error_code& ec, std::size_t) {
                                  HandleRequestHeaders(ec, socket, buffer);
                                });
                          } else {
                            // 关闭连接
                            socket->shutdown(tcp::socket::shutdown_both);
                            socket->close();
                          }
                        } else {
                          std::cerr << "Write failed: " << ec.message() << "\n";
                        }
                      });
  } else {
    std::cerr << "Response send failed: " << ec.message() << "\n";
  }
}

int main() {
  try {
    asio::io_context io_context;

    // 创建HTTP服务器
    HttpServer server(io_context, 80);  // HTTP标准端口为80
    server.Start();

    // 主线程也运行io_context
    io_context.run();

  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}