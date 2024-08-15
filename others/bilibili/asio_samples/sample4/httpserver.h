#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <asio.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "session.h"

class HttpServer {
 public:
  HttpServer(asio::io_context& io_context, unsigned short port);

  void Start();
  void RegisterHandler(const std::string& url,
                       const std::function<void(std::shared_ptr<Session>,
                                                std::string&, bool&)>& handler);

 private:
  void Accept();
  void HandleRequest(const asio::error_code& ec,
                     std::shared_ptr<Session> session,
                     std::shared_ptr<asio::streambuf> buffer);
  void SendResponse(std::shared_ptr<Session> session,
                    const std::string& response_data, bool keep_alive);
  void StartTimer(std::shared_ptr<Session> session);
  void CancelTimer(std::shared_ptr<Session> session);
  void SendDefaultResponse(std::shared_ptr<Session> session);

  asio::io_context& io_context_;
  asio::ip::tcp::acceptor acceptor_;
  std::unordered_map<std::string, std::function<void(std::shared_ptr<Session>,
                                                     std::string&, bool&)>>
      handlers_;
};

#endif