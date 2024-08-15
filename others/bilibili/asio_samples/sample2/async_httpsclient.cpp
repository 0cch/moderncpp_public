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

#include "async_httpsclient.h"

AsyncHttpsClient::AsyncHttpsClient(asio::io_context& io_context,
                                   asio::ssl::context& ssl_context,
                                   const std::string& server,
                                   const std::string& path)
    : resolver_(io_context),
      socket_(io_context, ssl_context),
      server_(server),
      path_(path) {}

void AsyncHttpsClient::Start() { Resolve(); }

void AsyncHttpsClient::Resolve() {
  resolver_.async_resolve(
      server_, "https",
      [this](const asio::error_code& ec, tcp::resolver::results_type results) {
        if (!ec) {
          Connect(results);
        } else {
          std::cerr << "Resolve failed: " << ec.message() << "\n";
        }
      });
}

void AsyncHttpsClient::Connect(const tcp::resolver::results_type& endpoints) {
  asio::async_connect(
      socket_.lowest_layer(), endpoints,
      [this](const asio::error_code& ec, const tcp::endpoint& endpoint) {
        if (!ec) {
          Handshake();
        } else {
          std::cerr << "Connect failed: " << ec.message() << "\n";
        }
      });
}

void AsyncHttpsClient::Handshake() {
  socket_.async_handshake(
      asio::ssl::stream_base::client, [this](const asio::error_code& ec) {
        if (!ec) {
          SendRequest();
        } else {
          std::cerr << "Handshake failed: " << ec.message() << "\n";
        }
      });
}

void AsyncHttpsClient::SendRequest() {
  std::ostream request_stream(&request_);
  request_stream << "GET " << path_ << " HTTP/1.1\r\n";
  request_stream << "Host: " << server_ << "\r\n";
  request_stream << "Accept: */*\r\n";
  request_stream << "Connection: close\r\n\r\n";

  asio::async_write(socket_, request_,
                    [this](const asio::error_code& ec, std::size_t) {
                      if (!ec) {
                        ReceiveResponse();
                      } else {
                        std::cerr << "Write failed: " << ec.message() << "\n";
                      }
                    });
}

void AsyncHttpsClient::ReceiveResponse() {
  asio::async_read(socket_, response_,
                   [this](const asio::error_code& ec, std::size_t) {
                     if (!ec || ec == asio::error::eof) {
                       std::cout << &response_;
                     } else {
                       std::cerr << "Read failed: " << ec.message() << "\n";
                     }
                   });
}

void AddWindowsRootCerts(asio::ssl::context& ctx) {
  HCERTSTORE sys_store = CertOpenSystemStore(0, "ROOT");
  if (!sys_store) {
    throw std::runtime_error("Unable to open system certificate store");
  }

  X509_STORE* store = X509_STORE_new();
  PCCERT_CONTEXT cert_context = NULL;
  while ((cert_context =
              CertEnumCertificatesInStore(sys_store, cert_context)) != NULL) {
    X509* x509 =
        d2i_X509(NULL, (const unsigned char**)&cert_context->pbCertEncoded,
                 cert_context->cbCertEncoded);
    if (x509) {
      X509_STORE_add_cert(store, x509);
      X509_free(x509);
    }
  }
  CertFreeCertificateContext(cert_context);
  CertCloseStore(sys_store, 0);
  SSL_CTX_set_cert_store(ctx.native_handle(), store);
}

int main() {
  try {
    asio::io_context io_context;
    asio::ssl::context ssl_context(asio::ssl::context::sslv23);

#ifdef _WIN32
    AddWindowsRootCerts(ssl_context);
#else
    ssl_context.set_default_verify_paths();
#endif
    ssl_context.set_verify_mode(asio::ssl::verify_peer);

    std::string server = "example.com";
    std::string path = "/";

    AsyncHttpsClient client(io_context, ssl_context, server, path);
    client.Start();
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}