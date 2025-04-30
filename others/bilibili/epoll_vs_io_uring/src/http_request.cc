// HTTP请求类实现
#include "http_request.h"

#include <string.h>
#include <algorithm>

HttpRequest::HttpRequest()
    : method_(HttpMethod::kUnknown),
      state_(HttpRequestParseState::kRequestLine) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::Reset() {
  method_ = HttpMethod::kUnknown;
  path_.clear();
  version_.clear();
  headers_.clear();
  body_.clear();
  state_ = HttpRequestParseState::kRequestLine;
}

bool HttpRequest::Parse(const char* buffer, size_t size) {
  const char* begin = buffer;
  const char* end = buffer + size;

  while (begin < end && state_ != HttpRequestParseState::kComplete &&
         state_ != HttpRequestParseState::kError) {
    if (state_ == HttpRequestParseState::kRequestLine) {
      const char* line_end = std::search(begin, end, "\r\n", "\r\n" + 2);
      if (line_end == end) {
        return false;  // 数据不完整，等待更多数据
      }

      if (!ParseRequestLine(begin, line_end)) {
        state_ = HttpRequestParseState::kError;
        return false;
      }

      begin = line_end + 2;  // 跳过\r\n
      state_ = HttpRequestParseState::kHeaders;
    } else if (state_ == HttpRequestParseState::kHeaders) {
      const char* line_end = std::search(begin, end, "\r\n", "\r\n" + 2);
      if (line_end == end) {
        return false;  // 数据不完整，等待更多数据
      }

      // 空行表示头部结束
      if (begin == line_end) {
        begin = line_end + 2;  // 跳过\r\n

        // 检查是否有请求体
        auto it = headers_.find("Content-Length");
        if (it != headers_.end() && std::stoi(it->second) > 0) {
          state_ = HttpRequestParseState::kBody;
        } else {
          state_ = HttpRequestParseState::kComplete;
        }
      } else {
        if (!ParseHeaders(begin, line_end)) {
          state_ = HttpRequestParseState::kError;
          return false;
        }
        begin = line_end + 2;  // 跳过\r\n
      }
    } else if (state_ == HttpRequestParseState::kBody) {
      auto it = headers_.find("Content-Length");
      if (it == headers_.end()) {
        state_ = HttpRequestParseState::kComplete;
        continue;
      }

      int content_length = std::stoi(it->second);
      if (end - begin >= content_length) {
        if (!ParseBody(begin, begin + content_length)) {
          state_ = HttpRequestParseState::kError;
          return false;
        }
        begin += content_length;
        state_ = HttpRequestParseState::kComplete;
      } else {
        return false;  // 数据不完整，等待更多数据
      }
    }
  }

  return state_ == HttpRequestParseState::kComplete;
}

bool HttpRequest::ParseRequestLine(const char* begin, const char* end) {
  // 查找第一个空格，分割方法和URL
  const char* space1 = std::find(begin, end, ' ');
  if (space1 == end) {
    return false;
  }

  // 解析HTTP方法
  std::string method(begin, space1);
  if (method == "GET") {
    method_ = HttpMethod::kGet;
  } else if (method == "POST") {
    method_ = HttpMethod::kPost;
  } else if (method == "PUT") {
    method_ = HttpMethod::kPut;
  } else if (method == "DELETE") {
    method_ = HttpMethod::kDelete;
  } else {
    method_ = HttpMethod::kUnknown;
  }

  // 查找第二个空格，分割URL和HTTP版本
  const char* space2 = std::find(space1 + 1, end, ' ');
  if (space2 == end) {
    return false;
  }

  // 解析URL
  path_.assign(space1 + 1, space2);

  // 解析HTTP版本
  version_.assign(space2 + 1, end);

  return true;
}

bool HttpRequest::ParseHeaders(const char* begin, const char* end) {
  // 查找冒号，分割头部字段名和值
  const char* colon = std::find(begin, end, ':');
  if (colon == end) {
    return false;
  }

  std::string key(begin, colon);
  // 跳过冒号和空格
  const char* value_begin = colon + 1;
  while (value_begin < end && *value_begin == ' ') {
    ++value_begin;
  }

  std::string value(value_begin, end);
  headers_[key] = value;

  return true;
}

bool HttpRequest::ParseBody(const char* begin, const char* end) {
  body_.assign(begin, end);
  return true;
}

HttpMethod HttpRequest::GetMethod() const {
  return method_;
}

const std::string& HttpRequest::GetPath() const {
  return path_;
}

const std::string& HttpRequest::GetVersion() const {
  return version_;
}

const std::string& HttpRequest::GetHeader(const std::string& key) const {
  static const std::string empty_string;
  auto it = headers_.find(key);
  return it != headers_.end() ? it->second : empty_string;
}

const std::string& HttpRequest::GetBody() const {
  return body_;
}

bool HttpRequest::IsComplete() const {
  return state_ == HttpRequestParseState::kComplete;
}