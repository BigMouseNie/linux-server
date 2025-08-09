#ifndef HTTP_HTTPPARSRWRAP_H_
#define HTTP_HTTPPARSRWRAP_H_

#include <string>
#include <unordered_map>

#include "Uri.h"
#include "http_parser.h"

class HttpParserWrap {
 public:
  HttpParserWrap();
  ~HttpParserWrap();
  HttpParserWrap(const HttpParserWrap& other);
  HttpParserWrap& operator=(const HttpParserWrap& other);

  int Parser(const std::string& data);
  int Method(int& method) const {
    if (method_ == -1) return -1;
    method = method_;
    return 0;
  }
  int Headers(const std::string& hdr_key, std::string& hdr_val) const;
  int Status(int status) const {
    if (status_.empty()) return -1;
    status = std::stoi(status_);
    return 0;
  }
  int Url(std::string& url) const {
    if (url_.empty()) return -1;
    url = url_;
    return 0;
  }
  int Body(std::string& body) {
    if (body_.empty()) return -1;
    body = body_;
    return 0;
  }

  std::string GetError() {
    const char* name = http_errno_name((enum http_errno)parser_.http_errno);
    const char* desc =
        http_errno_description((enum http_errno)parser_.http_errno);
    if (!name || !desc) return "";
    return std::string("[") + name + "] " + desc;
  }

 private:
  static int OnMessageBegin(http_parser* h_parser);
  static int OnUrl(http_parser* h_parser, const char* at, size_t length);
  static int OnStatus(http_parser* h_parser, const char* at, size_t length);
  static int OnHeaderField(http_parser* h_parser, const char* at,
                           size_t length);
  static int OnHeaderValue(http_parser* h_parser, const char* at,
                           size_t length);
  static int OnHeadersComplete(http_parser* h_parser);
  static int OnBody(http_parser* h_parser, const char* at, size_t length);
  static int OnMessageComplete(http_parser* h_parser);

  int OnMessageBegin();
  int OnUrl(const char* at, size_t length);
  int OnStatus(const char* at, size_t length);
  int OnHeaderField(const char* at, size_t length);
  int OnHeaderValue(const char* at, size_t length);
  int OnHeadersComplete();
  int OnBody(const char* at, size_t length);
  int OnMessageComplete();

  void Clear() {
    method_ = -1;
    url_.clear();
    status_.clear();
    body_.clear();
    complate_ = false;
    last_hdr_field_.clear();
    hdr_map_.clear();
  }

 private:
  int method_;
  std::string url_;
  std::string status_;
  std::string body_;
  bool complate_;
  std::string last_hdr_field_;
  std::unordered_map<std::string, std::string> hdr_map_;
  http_parser parser_;
  http_parser_settings settings_;
};

/**
 * https://example.com/search?q=chatgpt&lang=en#top
 * scheme: https
 * host: example.com
 * path: /search
 * query: q=chatgpt&lang=en
 * fragment: top
 */
class UrlObject {
 public:
  UrlObject(const std::string& uri) : error_str(nullptr) {
    valid_ = (Parser(uri) >= 0);
    GetQueryAndFillMap();
  }
  ~UrlObject() { uriFreeUriMembersA(&uri_); }

  std::string Scheme() const;
  std::string Host() const;
  std::string Path() const;
  std::string Query() const;
  bool IsValid() { return valid_; }
  std::string GetErrorStr() {
    if (error_str) {
      return std::string(error_str);
    }
    return "";
  }
  std::pair<bool, std::string> operator[](const std::string& key);

 private:
  int Parser(const std::string& uri);
  int DesParser(std::string& url);
  void GetQueryAndFillMap();

 private:
  UriUriA uri_;
  bool valid_;
  const char* error_str;
  std::string query_str_;
  std::unordered_map<std::string, std::string> query_map_;
};

#endif  // HTTP_HTTPPARSRWRAP_H_
