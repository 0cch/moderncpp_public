/**
 * 检测Windows环境宏_WIN32，设置_WIN32_WINNT为0x0601
 * 0x0601表示Windows 7
 */
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <wincrypt.h>
// clang-format on
#endif

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>

using asio::ip::tcp;

/**
 * @brief 向asio SSL上下文添加Windows根证书
 * @param ctx asio SSL上下文
 */
void AddWindowsRootCerts(asio::ssl::context& ctx) {
  // 打开系统的ROOT证书存储区
  HCERTSTORE sys_store = CertOpenSystemStore(0, "ROOT");
  if (!sys_store) {
    return;
  }
  // 创建一个新的X509存储区
  X509_STORE* store = X509_STORE_new();
  PCCERT_CONTEXT cert_context = NULL;
  // 遍历系统存储区中的所有证书
  while ((cert_context =
              CertEnumCertificatesInStore(sys_store, cert_context)) != NULL) {
    // 将证书从DER编码转换为X509证书
    X509* x509 =
        d2i_X509(NULL, (const unsigned char**)&cert_context->pbCertEncoded,
                 cert_context->cbCertEncoded);
    if (x509) {
      // 将X509证书添加到存储区
      X509_STORE_add_cert(store, x509);
      // 释放X509证书
      X509_free(x509);
    }
  }
  // 释放证书上下文
  CertFreeCertificateContext(cert_context);
  // 关闭存储区
  CertCloseStore(sys_store, 0);
  // 将存储区设置为asio SSL上下文的证书存储区
  SSL_CTX_set_cert_store(ctx.native_handle(), store);
}

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
 * @brief 发送HTTPS请求
 * @param server 服务器地址
 * @param path 请求路径
 * @details
 * 此函数创建一个SSL上下文，解析服务器地址，创建一个SSL流，进行握手，然后处理请求。
 */
void HttpsRequest(const std::string& server, const std::string& path) {
  // 创建I/O上下文
  asio::io_context io_context;
  // 创建SSL上下文，使用SSLv23版本
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);
  // 设置默认的证书路径
  ssl_context.set_default_verify_paths();
  // 设置验证模式为对等验证
  ssl_context.set_verify_mode(asio::ssl::verify_peer);
#ifdef _WIN32
  // 添加Windows根证书
  AddWindowsRootCerts(ssl_context);
#endif

  // 创建解析器，解析服务器地址
  tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(server, "https");

  // 创建SSL流
  asio::ssl::stream<tcp::socket> socket(io_context, ssl_context);

  // 连接到端点
  asio::connect(socket.lowest_layer(), endpoints);

  // 握手，客户端模式
  socket.handshake(asio::ssl::stream_base::client);

  // 处理请求
  HandleRequest(socket, server, path);
}

int main() {
  const std::string server = "example.com";
  const std::string path = "/";

  try {
    HttpsRequest(server, path);
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
