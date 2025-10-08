#ifndef HTTP_HTTPPARSERWRAP_H_
#define HTTP_HTTPPARSERWRAP_H_

#include <string.h>

#include <string>
#include <vector>

#include "http_parser.h"
#include "ring_buffer.h"

struct StrRange {
  StrRange() : start(0), length(0) {}
  size_t start;
  size_t length;
};

struct HdrFV {
  StrRange field;
  StrRange value;
};

template <typename T>
class HttpParserWrap {
 public:
  HttpParserWrap();
  ~HttpParserWrap();
  int SetData(const T* data, http_parser_type type);
  int Parser();
  const char* Url(size_t& length) const {
    length = url_.length;
    return length == 0 ? nullptr : GetReadPtr() + url_.start;
  }

  std::string Url(bool* is_exist = nullptr) const {
    if (url_.length == 0) {
      if (is_exist) *is_exist = false;
      return "";
    };
    if (is_exist) *is_exist = true;
    return std::string(GetReadPtr() + url_.start, url_.length);
  }

  http_method Method() const {
    return static_cast<http_method>(parser_.method);
  }

  const char* Status(size_t& length) const {
    length = status_.length;
    return length == 0 ? nullptr : GetReadPtr() + status_.start;
  }

  std::string Status(bool* is_exist = nullptr) const {
    if (status_.length == 0) {
      if (is_exist) *is_exist = false;
      return "";
    }
    if (is_exist) *is_exist = true;
    return std::string(GetReadPtr() + status_.start, status_.length);
  }

  int StatusCode() const { return parser_.status_code; }

  const char* Headers(const char* field, size_t& val_len) const;
  std::string Headers(const std::string& field, bool* is_exist = nullptr) const;

  const char* Body(size_t& length) {
    length = body_.length;
    return length == 0 ? nullptr : GetReadPtr() + body_.start;
  }

  std::string Body(bool* is_exist = nullptr) const {
    if (body_.length == 0) {
      if (is_exist) *is_exist = false;
      return "";
    }
    if (is_exist) *is_exist = true;
    return std::string(GetReadPtr() + body_.start, body_.length);
  }

  bool HeaderComplete() const { return hdr_complate_; }

  bool Complete() const { return complate_; }

 private:
  void Reset(http_parser_type type);
  const char* GetReadPtr() const;
  size_t Readable() const;

  inline static int OnMessageBegin(http_parser* h_parser) {
    return ((HttpParserWrap*)(h_parser->data))->OnMessageBegin();
  }

  inline static int OnUrl(http_parser* h_parser, const char* at,
                          size_t length) {
    return ((HttpParserWrap*)(h_parser->data))->OnUrl(at, length);
  }

  inline static int OnStatus(http_parser* h_parser, const char* at,
                             size_t length) {
    return ((HttpParserWrap*)(h_parser->data))->OnStatus(at, length);
  }

  inline static int OnHeaderField(http_parser* h_parser, const char* at,
                                  size_t length) {
    return ((HttpParserWrap*)(h_parser->data))->OnHeaderField(at, length);
  }

  inline static int OnHeaderValue(http_parser* h_parser, const char* at,
                                  size_t length) {
    return ((HttpParserWrap*)(h_parser->data))->OnHeaderValue(at, length);
  }

  inline static int OnHeadersComplete(http_parser* h_parser) {
    return ((HttpParserWrap*)(h_parser->data))->OnHeadersComplete();
  }

  inline static int OnBody(http_parser* h_parser, const char* at,
                           size_t length) {
    return ((HttpParserWrap*)(h_parser->data))->OnBody(at, length);
  }

  inline static int OnMessageComplete(http_parser* h_parser) {
    return ((HttpParserWrap*)(h_parser->data))->OnMessageComplete();
  }

  int OnMessageBegin();
  int OnUrl(const char* at, size_t length);
  int OnStatus(const char* at, size_t length);
  int OnHeaderField(const char* at, size_t length);
  int OnHeaderValue(const char* at, size_t length);
  int OnHeadersComplete();
  int OnBody(const char* at, size_t length);
  int OnMessageComplete();

 private:
  bool complate_;
  bool hdr_complate_;
  bool cur_hdr_status_;
  size_t offset_;
  StrRange url_;
  StrRange status_;
  StrRange body_;
  const T* data_;
  http_parser parser_;
  http_parser_settings settings_;
  std::vector<HdrFV> hdr_fv_;
};

template <typename T>
HttpParserWrap<T>::HttpParserWrap()
    : complate_(false),
      hdr_complate_(false),
      cur_hdr_status_(false),
      offset_(0),
      data_(nullptr) {
  http_parser_settings_init(&settings_);
  settings_.on_message_begin = &HttpParserWrap<T>::OnMessageBegin;
  settings_.on_url = &HttpParserWrap<T>::OnUrl;
  settings_.on_status = &HttpParserWrap<T>::OnStatus;
  settings_.on_header_field = &HttpParserWrap<T>::OnHeaderField;
  settings_.on_header_value = &HttpParserWrap<T>::OnHeaderValue;
  settings_.on_headers_complete = &HttpParserWrap<T>::OnHeadersComplete;
  settings_.on_body = &HttpParserWrap<T>::OnBody;
  settings_.on_message_complete = &HttpParserWrap<T>::OnMessageComplete;
}

template <typename T>
HttpParserWrap<T>::~HttpParserWrap() {
  Reset(http_parser_type::HTTP_REQUEST);
}

template <typename T>
int HttpParserWrap<T>::SetData(const T* data, http_parser_type type) {
  Reset(type);
  if (!data) return -1;
  data_ = data;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::Parser() {
  if (!data_) return -1;
  size_t nparsed = http_parser_execute(
      &parser_, &settings_, GetReadPtr() + offset_, Readable() - offset_);
  if (nparsed != Readable() - offset_ &&
      HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {
    // log
    return -2;
  }

  offset_ += nparsed;
  return static_cast<int>(nparsed);
}

template <typename T>
const char* HttpParserWrap<T>::Headers(const char* field,
                                       size_t& val_len) const {
  val_len = 0;
  if (!field) return nullptr;
  size_t len = strlen(field);
  for (const auto& fv : hdr_fv_) {
    if (len == fv.field.length &&
        strncmp(field, GetReadPtr() + fv.field.start, len) == 0) {
      val_len = fv.value.length;
      return GetReadPtr() + fv.value.start;
    }
  }
  return nullptr;
}

template <typename T>
std::string HttpParserWrap<T>::Headers(const std::string& field,
                                       bool* is_exist) const {
  size_t val_len = 0;
  const char* value = Headers(field.c_str(), val_len);
  if (value) {
    if (is_exist) *is_exist = true;
    return std::string(value, val_len);
  }
  if (is_exist) *is_exist = false;
  return "";
}

template <typename T>
void HttpParserWrap<T>::Reset(http_parser_type type) {
  complate_ = false;
  hdr_complate_ = false;
  cur_hdr_status_ = false;
  offset_ = 0;
  data_ = nullptr;
  http_parser_init(&parser_, type);
  parser_.data = this;
  hdr_fv_.clear();
}

template <typename T>
const char* HttpParserWrap<T>::GetReadPtr() const {
  return data_->GetReadPtr();
}

template <>
const char* HttpParserWrap<std::string>::GetReadPtr() const;

template <typename T>
size_t HttpParserWrap<T>::Readable() const {
  return data_->Readable();
}

template <>
size_t HttpParserWrap<std::string>::Readable() const;

template <typename T>
int HttpParserWrap<T>::OnMessageBegin() {
  // pass
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnUrl(const char* at, size_t length) {
  if (url_.length == 0) url_.start = at - GetReadPtr();
  url_.length += length;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnStatus(const char* at, size_t length) {
  if (status_.length == 0) status_.start = at - GetReadPtr();
  status_.length += length;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnHeaderField(const char* at, size_t length) {
  if (!cur_hdr_status_) {
    hdr_fv_.emplace_back(HdrFV());
    hdr_fv_.back().field.start = at - GetReadPtr();
    cur_hdr_status_ = true;
  }
  hdr_fv_.back().field.length += length;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnHeaderValue(const char* at, size_t length) {
  cur_hdr_status_ = false;
  if (hdr_fv_.back().value.length == 0)
    hdr_fv_.back().value.start = at - GetReadPtr();
  hdr_fv_.back().value.length += length;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnHeadersComplete() {
  hdr_complate_ = true;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnBody(const char* at, size_t length) {
  if (body_.length == 0) body_.start = at - GetReadPtr();
  body_.length += length;
  return 0;
}

template <typename T>
int HttpParserWrap<T>::OnMessageComplete() {
  complate_ = true;
  return 0;
}

#endif  // HTTP_HTTPPARSERWRAP_H_
