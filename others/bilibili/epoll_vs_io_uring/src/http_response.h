// HTTP响应类声明
#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <map>
#include <string>

// HTTP状态码
enum class HttpStatusCode {
  k200Ok = 200,
  k400BadRequest = 400,
  k404NotFound = 404,
  k500InternalError = 500
};

// HTTP响应类
class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  // 设置状态码
  void SetStatusCode(HttpStatusCode code);

  // 设置响应头
  void SetHeader(const std::string& key, const std::string& value);

  // 设置响应体
  void SetBody(const std::string& body);

  // 生成完整的HTTP响应
  std::string ToString() const;

 private:
  // 获取状态码对应的文本描述
  std::string GetStatusMessage(HttpStatusCode code) const;

  HttpStatusCode status_code_;                  // HTTP状态码
  std::map<std::string, std::string> headers_;  // 响应头
  std::string body_;                            // 响应体
};

#endif  // HTTP_RESPONSE_H_