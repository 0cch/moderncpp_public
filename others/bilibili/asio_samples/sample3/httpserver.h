#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using asio::ip::tcp;

class HttpServer {
 public:
  HttpServer(asio::io_context& io_context, unsigned short port);

  void Start();

 private:
  void Accept();
  void HandleAccept(const asio::error_code& ec,
                    std::shared_ptr<tcp::socket> socket);
  void HandleRequestHeaders(const asio::error_code& ec,
                            std::shared_ptr<tcp::socket> socket,
                            std::shared_ptr<asio::streambuf> buffer);
  void HandleRequestBody(const asio::error_code& ec,
                         std::shared_ptr<tcp::socket> socket,
                         std::shared_ptr<asio::streambuf> buffer,
                         std::size_t content_length, bool keep_alive);
  void SendResponse(const asio::error_code& ec,
                    std::shared_ptr<tcp::socket> socket,
                    std::shared_ptr<asio::streambuf> response, bool keep_alive);

  asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

#endif  // ASYNC_HTTPS_SERVER_H
