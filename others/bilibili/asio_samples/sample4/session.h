#ifndef SESSION_H
#define SESSION_H

#include <llhttp.h>

#include <asio.hpp>
#include <memory>
#include <unordered_map>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(asio::io_context& io_context)
      : socket_(io_context), timer_(io_context) {
    llhttp_init(&parser_, HTTP_BOTH, &settings_);
    parser_.data = this;

    std::cout << "Session created: " << this
              << "  @tid: " << std::this_thread::get_id() << "\n";
  }

  ~Session() { std::cout << "Session destroyed: " << this << "\n"; }

  void reset() {
    llhttp_reset(&parser_);
    parser_.data = this;
    url_.clear();
    headers_.clear();
    body_.clear();
    is_complete_ = false;
  }

  static inline llhttp_settings_t settings_;

  std::string url_;
  std::unordered_map<std::string, std::string> headers_;
  std::string current_header_field_;
  std::string body_;
  bool is_complete_ = false;

  tcp::socket socket_;
  asio::steady_timer timer_;
  llhttp_t parser_;
};

#endif