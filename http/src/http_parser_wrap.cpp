#include "http_parser_wrap.h"

template <>
const char* HttpParserWrap<std::string>::GetReadPtr() const {
  return data_->c_str();
}

template <>
size_t HttpParserWrap<std::string>::Readable() const {
  return data_->size();
}
