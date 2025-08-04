#include "http_parser_wrap.h"

#include <sstream>

#include "memory.h"

HttpParserWrap::HttpParserWrap() : complate_(false) {
  memset(&parser_, 0, sizeof(parser_));
  parser_.data = this;
  http_parser_init(&parser_, HTTP_REQUEST);
  http_parser_settings_init(&settings_);
  settings_.on_message_begin = &HttpParserWrap::OnMessageBegin;
  settings_.on_url = &HttpParserWrap::OnUrl;
  settings_.on_status = &HttpParserWrap::OnStatus;
  settings_.on_header_field = &HttpParserWrap::OnHeaderField;
  settings_.on_header_value = &HttpParserWrap::OnHeaderValue;
  settings_.on_headers_complete = &HttpParserWrap::OnHeadersComplete;
  settings_.on_body = &HttpParserWrap::OnBody;
  settings_.on_message_complete = &HttpParserWrap::OnMessageBegin;
}

HttpParserWrap::~HttpParserWrap() {}

HttpParserWrap::HttpParserWrap(const HttpParserWrap& other)
    : url_(other.url_),
      status_(other.status_),
      body_(other.body_),
      complate_(other.complate_),
      last_hdr_field_(other.last_hdr_field_),
      hdr_map_(other.hdr_map_) {
  memcpy(&parser_, &(other.parser_), sizeof(&parser_));
  parser_.data = this;
  memcpy(&settings_, &(other.settings_), sizeof(&settings_));
}

HttpParserWrap& HttpParserWrap::operator=(const HttpParserWrap& other) {
  url_ = other.url_;
  status_ = other.status_;
  body_ = other.body_;
  complate_ = other.complate_;
  last_hdr_field_ = other.last_hdr_field_;
  hdr_map_ = other.hdr_map_;
  memcpy(&parser_, &(other.parser_), sizeof(&parser_));
  parser_.data = this;
  memcpy(&settings_, &(other.settings_), sizeof(&settings_));
}

int HttpParserWrap::Parser(const std::string& data) {
  complate_ = false;
  size_t ret =
      http_parser_execute(&parser_, &settings_, data.data(), data.size());
  if (complate_ == false) {
    parser_.http_errno = 0x70;
    return -1;
  }
  return ret;
}

int HttpParserWrap::OnMessageBegin(http_parser* h_parser) {
  return ((HttpParserWrap*)(h_parser->data))->OnMessageBegin();
}

int HttpParserWrap::OnUrl(http_parser* h_parser, const char* at,
                          size_t length) {
  return ((HttpParserWrap*)(h_parser->data))->OnUrl(at, length);
}

int HttpParserWrap::OnStatus(http_parser* h_parser, const char* at,
                             size_t length) {
  return ((HttpParserWrap*)(h_parser->data))->OnStatus(at, length);
}

int HttpParserWrap::OnHeaderField(http_parser* h_parser, const char* at,
                                  size_t length) {
  return ((HttpParserWrap*)(h_parser->data))->OnHeaderField(at, length);
}

int HttpParserWrap::OnHeaderValue(http_parser* h_parser, const char* at,
                                  size_t length) {
  return ((HttpParserWrap*)(h_parser->data))->OnHeaderValue(at, length);
}

int HttpParserWrap::OnHeadersComplete(http_parser* h_parser) {
  return ((HttpParserWrap*)(h_parser->data))->OnHeadersComplete();
}

int HttpParserWrap::OnBody(http_parser* h_parser, const char* at,
                           size_t length) {
  return ((HttpParserWrap*)(h_parser->data))->OnBody(at, length);
}

int HttpParserWrap::OnMessageComplete(http_parser* h_parser) {
  return ((HttpParserWrap*)(h_parser->data))->OnMessageComplete();
}

int HttpParserWrap::OnMessageBegin() {
  // pass
  return 0;
}

int HttpParserWrap::OnUrl(const char* at, size_t length) {
  url_.append(at, length);
  return 0;
}

int HttpParserWrap::OnStatus(const char* at, size_t length) {
  status_.append(at, length);
  return 0;
}

int HttpParserWrap::OnHeaderField(const char* at, size_t length) {
  last_hdr_field_.append(at, length);
  return 0;
}

int HttpParserWrap::OnHeaderValue(const char* at, size_t length) {
  hdr_map_[last_hdr_field_] = std::move(std::string(at, length));
  return 0;
}

int HttpParserWrap::OnHeadersComplete() { return 0; }

int HttpParserWrap::OnBody(const char* at, size_t length) {
  body_.append(at, length);
  return 0;
}

int HttpParserWrap::OnMessageComplete() {
  complate_ = true;
  return 0;
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
