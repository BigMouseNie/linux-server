#include "http_parser_wrap.h"

#include <sstream>

#include "memory.h"

template <>
const char* HttpParserWrap<std::string>::GetReadPtr() const {
  return data_->c_str();
}

template <>
size_t HttpParserWrap<std::string>::Readable() const {
  return data_->size();
}

std::string UrlObject::Scheme() const {
  if (!uri_.scheme.first) return "";
  return std::string(uri_.scheme.first,
                     uri_.scheme.afterLast - uri_.scheme.first);
}

std::string UrlObject::Host() const {
  if (!uri_.hostText.first) return "";
  return std::string(uri_.hostText.first,
                     uri_.hostText.afterLast - uri_.hostText.first);
}

std::string UrlObject::Path() const {
  std::string path;
  for (auto seg = uri_.pathHead; seg; seg = seg->next) {
    if (seg != uri_.pathHead) path += "/";
    if (seg->text.first) {
      path.append(seg->text.first, seg->text.afterLast - seg->text.first);
    }
  }
  return path;
}

std::string UrlObject::Query() const { return query_str_; }

void UrlObject::GetQueryAndFillMap() {
  if (!uri_.query.first) {
    query_str_ = "";
    return;
  };
  query_str_.append(uri_.query.first, uri_.query.afterLast - uri_.query.first);

  std::istringstream stream(query_str_);
  std::string pair;

  while (std::getline(stream, pair, '&')) {
    size_t eq_pos = pair.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = pair.substr(0, eq_pos);
      std::string value = pair.substr(eq_pos + 1);
      query_map_[key] = value;
    }
  }
}

int UrlObject::Parser(const std::string& uri) {
  memset(&uri_, 0, sizeof(uri_));
  if (uriParseSingleUriA(&uri_, uri.c_str(), &error_str) != URI_SUCCESS) {
    return -1;
  }
  return 0;
}

int UrlObject::DesParser(std::string& url_str) {
  int chars_required;
  if (uriToStringCharsRequiredA(&uri_, &chars_required) != URI_SUCCESS) {
    return -1;
  }
  url_str.resize(chars_required);
  if (uriToStringA(url_str.data(), &uri_, chars_required, NULL) !=
      URI_SUCCESS) {
    url_str.resize(0);
    return -2;
  }
  return 0;
}

std::pair<bool, std::string> UrlObject::operator[](const std::string& key) {
  auto it = query_map_.find(key);
  if (it == query_map_.end()) {
    return {false, ""};
  }
  return {true, it->second};
}
