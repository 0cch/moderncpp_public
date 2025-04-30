// HTTP请求类声明
#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <map>
#include <string>

// HTTP请求解析状态
enum class HttpRequestParseState {
  kRequestLine,  // 正在解析请求行
  kHeaders,      // 正在解析请求头
  kBody,         // 正在解析请求体
  kComplete,     // 解析完成
  kError         // 解析错误
};

// HTTP请求方法
enum class HttpMethod { kGet, kPost, kPut, kDelete, kUnknown };

// HTTP请求类
class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest();

  // 重置请求状态
  void Reset();

  // 解析HTTP请求
  bool Parse(const char* buffer, size_t size);

  // 获取HTTP方法
  HttpMethod GetMethod() const;

  // 获取请求路径
  const std::string& GetPath() const;

  // 获取HTTP版本
  const std::string& GetVersion() const;

  // 获取请求头
  const std::string& GetHeader(const std::string& key) const;

  // 获取请求体
  const std::string& GetBody() const;

  // 判断请求是否解析完成
  bool IsComplete() const;

 private:
  // 解析请求行
  bool ParseRequestLine(const char* begin, const char* end);

  // 解析请求头
  bool ParseHeaders(const char* begin, const char* end);

  // 解析请求体
  bool ParseBody(const char* begin, const char* end);

  HttpMethod method_;                           // HTTP方法
  std::string path_;                            // 请求路径
  std::string version_;                         // HTTP版本
  std::map<std::string, std::string> headers_;  // 请求头
  std::string body_;                            // 请求体
  HttpRequestParseState state_;                 // 解析状态
};

#endif  // HTTP_REQUEST_H_