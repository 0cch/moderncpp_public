// HTTP响应类实现
#include "http_response.h"

HttpResponse::HttpResponse() : status_code_(HttpStatusCode::k200Ok) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::SetStatusCode(HttpStatusCode code) {
  status_code_ = code;
}

void HttpResponse::SetHeader(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void HttpResponse::SetBody(const std::string& body) {
  body_ = body;
}

std::string HttpResponse::ToString() const {
  std::string response;

  // 添加状态行
  response.append("HTTP/1.1 ");
  response.append(std::to_string(static_cast<int>(status_code_)));
  response.append(" ");
  response.append(GetStatusMessage(status_code_));
  response.append("\r\n");

  // 添加响应头
  for (const auto& header : headers_) {
    response.append(header.first);
    response.append(": ");
    response.append(header.second);
    response.append("\r\n");
  }

  // 添加Content-Length头
  response.append("Content-Length: ");
  response.append(std::to_string(body_.size()));
  response.append("\r\n");

  // 空行分隔头部和响应体
  response.append("\r\n");

  // 添加响应体
  response.append(body_);

  return response;
}

std::string HttpResponse::GetStatusMessage(HttpStatusCode code) const {
  switch (code) {
    case HttpStatusCode::k200Ok:
      return "OK";
    case HttpStatusCode::k400BadRequest:
      return "Bad Request";
    case HttpStatusCode::k404NotFound:
      return "Not Found";
    case HttpStatusCode::k500InternalError:
      return "Internal Server Error";
    default:
      return "Unknown";
  }
}