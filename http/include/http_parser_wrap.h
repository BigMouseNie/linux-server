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
  //   uint32_t Method() const;
  //   void Headers();
  //   void Status();
  //   void Url();
  //   void Body();
  //   void Errno();

 protected:
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

 private:
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
