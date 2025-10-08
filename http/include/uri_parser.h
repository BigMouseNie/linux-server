#ifndef HTTP_URIPARSER_H_
#define HTTP_URIPARSER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "Uri.h"

/**
 * https://user:pass@[2001:db8::1]:8080/dir1/dir2/file.html;param?key=value&x=42#section-2
 * scheme: https
 * userinfo: user:pass
 * host: [2001:db8::1]
 * port: 8080
 * path: /dir1/dir2/file.html;param
 * query: key=value&x=42
 * fragment: section-2
 */

class UriParser {
  using KV = std::unordered_map<std::string, std::string>;

 public:
  explicit UriParser(const std::string& uri_str) : uri_str_(uri_str) {
    valid_ = (Parser(uri_str_) >= 0);
    ParserQuery();
    ParserPath();
  }
  ~UriParser() { uriFreeUriMembersA(&uri_); }

  std::string Scheme() const {
    if (!uri_.scheme.first) return "";
    return std::string(uri_.scheme.first,
                       uri_.scheme.afterLast - uri_.scheme.first);
  }

  std::string UserInfo() const {
    if (!uri_.userInfo.first) return "";
    return std::string(uri_.userInfo.first,
                       uri_.userInfo.afterLast - uri_.userInfo.first);
  }

  std::string Host() const {
    if (!uri_.hostText.first) return "";
    return std::string(uri_.hostText.first,
                       uri_.hostText.afterLast - uri_.hostText.first);
  }

  std::string Port() const {
    if (!uri_.portText.first) return "";
    return std::string(uri_.portText.first,
                       uri_.portText.afterLast - uri_.portText.first);
  }

  std::string Path() const;

  std::string PathWithParams() const;

  std::string Query() const {
    if (!uri_.query.first) return "";
    return std::string(uri_.query.first,
                       uri_.query.afterLast - uri_.query.first);
  }

  std::string Fragment() const {
    if (!uri_.fragment.first) return "";
    return std::string(uri_.fragment.first,
                       uri_.fragment.afterLast - uri_.fragment.first);
  }

  bool IsValid() { return valid_; }

  std::string GetValByQuery(const std::string& key) const;

  const KV& GetQueryMap() const { return query_map_; }
  const std::vector<std::pair<std::string, KV>>& GetPathParamsKV() const {
    return path_params_kv_;
  }
  const std::vector<std::pair<std::string, std::string>>& GetPathParams()
      const {
    return path_params_;
  }

 private:
  int Parser(const std::string& uri);
  void ParserQuery();
  void ParserPath();
  void ParseMatrixSegment(const std::string& segment);

 private:
  bool valid_;
  UriUriA uri_;
  std::string uri_str_;
  KV query_map_;
  std::vector<std::pair<std::string, KV>> path_params_kv_;
  std::vector<std::pair<std::string, std::string>> path_params_;
};

#endif  // HTTP_URLPARSR_H_
