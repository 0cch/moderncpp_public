#ifndef ASYNC_HTTP_CLIENT_H
#define ASYNC_HTTP_CLIENT_H

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <memory>
#include <string>
using asio::ip::tcp;
/**
 * @brief 异步HTTPS客户端类
 *
 * 该类实现了一个异步的HTTPS客户端，可以用于异步地向服务器发送HTTPS请求并接收响应。
 */
class AsyncHttpsClient {
 public:
  /**
   * @brief 构造函数
   *
   * @param io_context 异步I/O上下文
   * @param ssl_context SSL上下文
   * @param server 服务器地址
   * @param path 请求路径
   */
  AsyncHttpsClient(asio::io_context& io_context,
                   asio::ssl::context& ssl_context, const std::string& server,
                   const std::string& path);
  /**
   * @brief 开始执行
   *
   * 该函数用于开始执行异步HTTPS请求。
   */
  void Start();

 private:
  // 解析服务器地址
  void Resolve();
  // 连接到服务器
  void Connect(const tcp::resolver::results_type& endpoints);
  // 执行SSL握手
  void Handshake();
  // 发送HTTPS请求
  void SendRequest();
  // 接收HTTPS响应
  void ReceiveResponse();
  // 域名解析器
  tcp::resolver resolver_;
  // SSL socket
  asio::ssl::stream<tcp::socket> socket_;
  // 服务器地址
  std::string server_;
  // 请求路径
  std::string path_;
  // 请求内容
  asio::streambuf request_;
  // 响应内容
  asio::streambuf response_;
};

#endif  // ASYNC_HTTP_CLIENT_H
