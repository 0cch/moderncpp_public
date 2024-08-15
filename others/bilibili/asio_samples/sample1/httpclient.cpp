/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <asio.hpp>
#include <iostream>

using asio::ip::tcp;

/**
 * @brief 处理HTTP请求
 * @tparam T 模板参数，表示socket的类型
 * @param socket 要处理的socket
 * @param server 服务器地址
 * @param path 请求路径
 *
 * HandleRequest函数用于构造一个HTTP GET请求，并通过socket发送给服务器。
 */
template <class T>
void HandleRequest(T& socket, const std::string& server,
                   const std::string& path) {
  // 构造HTTP GET请求
  std::string request = "GET " + path + " HTTP/1.1\r\n";
  request += "Host: " + server + "\r\n";
  request += "Accept: */*\r\n";
  request += "Connection: close\r\n\r\n";

  // 通过socket发送请求
  asio::write(socket, asio::buffer(request));

  char buffer[1024]{};
  asio::error_code error;
  // 读取服务器的响应
  while (size_t len = socket.read_some(asio::buffer(buffer), error)) {
    std::cout.write(buffer, len);
  }
}

/**
 * @brief 发送HTTP请求
 * @param server 服务器地址
 * @param path 请求路径
 *
 * HttpRequest函数创建一个io_context，解析服务器地址，创建一个socket，
 * 连接到服务器，然后调用HandleRequest发送请求。
 */
void HttpRequest(const std::string& server, const std::string& path) {
  asio::io_context io_context;

  // 解析服务器地址
  tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(server, "http");

  // 创建一个socket
  tcp::socket socket(io_context);

  // 连接到服务器
  asio::connect(socket, endpoints);

  // 发送请求
  HandleRequest(socket, server, path);
}

int main() {
  const std::string server = "127.0.0.1";
  const std::string path = "/";

  try {
    HttpRequest(server, path);
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
