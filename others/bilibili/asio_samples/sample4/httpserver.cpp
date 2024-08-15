#include "httpserver.h"

HttpServer::HttpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.listen();

  // 初始化 llhttp 设置
  llhttp_settings_init(&Session::settings_);
  Session::settings_.on_url = [](llhttp_t* parser, const char* at,
                                 size_t length) {
    auto* session = static_cast<Session*>(parser->data);
    session->url_.assign(at, length);
    return 0;
  };

  Session::settings_.on_header_field = [](llhttp_t* parser, const char* at,
                                          size_t length) {
    auto* session = static_cast<Session*>(parser->data);
    session->current_header_field_.assign(at, length);
    return 0;
  };

  Session::settings_.on_header_value = [](llhttp_t* parser, const char* at,
                                          size_t length) {
    auto* session = static_cast<Session*>(parser->data);
    session->headers_[session->current_header_field_] = std::string(at, length);
    return 0;
  };

  Session::settings_.on_body = [](llhttp_t* parser, const char* at,
                                  size_t length) {
    auto* session = static_cast<Session*>(parser->data);
    session->body_.assign(at, length);
    return 0;
  };

  Session::settings_.on_message_complete = [](llhttp_t* parser) {
    auto* session = static_cast<Session*>(parser->data);
    session->is_complete_ = true;
    return 0;
  };
}

void HttpServer::Start() { Accept(); }

void HttpServer::Accept() {
  auto session = std::make_shared<Session>(io_context_);
  acceptor_.async_accept(
      session->socket_, [this, session](const asio::error_code& ec) {
        if (!ec) {
          auto buffer = std::make_shared<asio::streambuf>();
          StartTimer(session);
          asio::async_read_until(
              session->socket_, *buffer, "\r\n\r\n",
              [this, session, buffer](const asio::error_code& ec, std::size_t) {
                CancelTimer(session);
                HandleRequest(ec, session, buffer);
              });
        }
        Accept();
      });
}

void HttpServer::HandleRequest(const asio::error_code& ec,
                               std::shared_ptr<Session> session,
                               std::shared_ptr<asio::streambuf> buffer) {
  if (!ec) {
    std::istream request_stream(buffer.get());
    std::string request_data((std::istreambuf_iterator<char>(request_stream)),
                             std::istreambuf_iterator<char>());

    if (session->is_complete_) {
      session->reset();
    }

    llhttp_execute(&session->parser_, request_data.data(), request_data.size());

    if (session->is_complete_) {
      auto handler = handlers_.find(session->url_);
      if (handler != handlers_.end()) {
        std::string response_data;
        bool keep_alive = true;
        if (session->headers_.count("Connection:") &&
            session->headers_["Connection:"] == "close") {
          keep_alive = false;
        }
        handler->second(session, response_data, keep_alive);
        SendResponse(session, response_data, keep_alive);
      } else {
        SendDefaultResponse(session);
      }
    } else {
      if (session->headers_.count("Content-Length")) {
        std::size_t content_length =
            std::stoul(session->headers_["Content-Length"]);
        StartTimer(session);
        asio::async_read(session->socket_, *buffer,
                         asio::transfer_exactly(content_length),
                         [this, session, buffer, content_length](
                             const asio::error_code& ec, std::size_t) {
                           CancelTimer(session);
                           HandleRequest(ec, session, buffer);
                         });
      } else {
        SendDefaultResponse(session);
      }
    }
  } else {
    std::cout << "Handle request error: " << ec.message() << "\n";
  }
}

void HttpServer::SendResponse(std::shared_ptr<Session> session,
                              const std::string& response_data,
                              bool keep_alive) {
  auto response = std::make_shared<asio::streambuf>();
  std::ostream response_stream(response.get());

  response_stream << response_data;

  StartTimer(session);
  asio::async_write(session->socket_, *response,
                    [this, session, response, keep_alive](
                        const asio::error_code& ec, std::size_t) {
                      CancelTimer(session);
                      if (!ec) {
                        if (keep_alive) {
                          // 继续读取新的请求
                          auto buffer = std::make_shared<asio::streambuf>();
                          asio::async_read_until(
                              session->socket_, *buffer, "\r\n\r\n",
                              [this, session, buffer](
                                  const asio::error_code& ec, std::size_t) {
                                HandleRequest(ec, session, buffer);
                              });
                        } else {
                          // 关闭连接
                          session->socket_.shutdown(tcp::socket::shutdown_both);
                          session->socket_.close();
                        }
                      } else {
                        std::cerr << "Write failed: " << ec.message() << "\n";
                      }
                    });
}

void HttpServer::StartTimer(std::shared_ptr<Session> session) {
  session->timer_.expires_after(std::chrono::seconds(30));
  session->timer_.async_wait([this, session](const asio::error_code& ec) {
    if (!ec) {
      session->socket_.cancel();
      std::cerr << "Timeout, closing socket.\n";
      session->socket_.shutdown(tcp::socket::shutdown_both);
      session->socket_.close();
    }
  });
}

void HttpServer::CancelTimer(std::shared_ptr<Session> session) {
  session->timer_.cancel();
}

void HttpServer::SendDefaultResponse(std::shared_ptr<Session> session) {
  std::string response_data =
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Length: 13\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "\r\n"
      "404 Not Found";
  SendResponse(session, response_data, false);
}

void HttpServer::RegisterHandler(
    const std::string& url,
    const std::function<void(std::shared_ptr<Session>, std::string&, bool&)>&
        handler) {
  handlers_[url] = handler;
}
