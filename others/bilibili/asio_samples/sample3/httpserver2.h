#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(asio::io_context& io_context)
      : socket_(io_context), timer_(io_context) {
    std::cout << "Session created: " << this
              << "  @tid: " << std::this_thread::get_id() << "\n";
  }
  ~Session() { std::cout << "Session destroyed: " << this << "\n"; }

  tcp::socket& socket() { return socket_; }
  asio::steady_timer& timer() { return timer_; }

 private:
  tcp::socket socket_;
  asio::steady_timer timer_;
};

class HttpServer {
 public:
  HttpServer(asio::io_context& io_context, unsigned short port);

  void Start();

 private:
  void Accept();
  void HandleAccept(const asio::error_code& ec,
                    std::shared_ptr<Session> socket);
  void HandleRequestHeaders(const asio::error_code& ec,
                            std::shared_ptr<Session> socket,
                            std::shared_ptr<asio::streambuf> buffer);
  void HandleRequestBody(const asio::error_code& ec,
                         std::shared_ptr<Session> socket,
                         std::shared_ptr<asio::streambuf> buffer,
                         std::size_t content_length, bool keep_alive);
  void SendResponse(const asio::error_code& ec, std::shared_ptr<Session> socket,
                    std::shared_ptr<asio::streambuf> response, bool keep_alive);
  void StartTimer(std::shared_ptr<Session> session);
  void CancelTimer(std::shared_ptr<Session> session);

  asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

#endif  // ASYNC_HTTPS_SERVER_H
