#include "field.h"

template <>
void Field<int>::SetVal(DataElem& val, const std::string& val_str) {
  val = std::stoi(val_str);
}

template <>
void Field<double>::SetVal(DataElem& val, const std::string& val_str) {
  val = std::stoll(val_str);
}

template <>
void Field<bool>::SetVal(DataElem& val, const std::string& val_str) {
  val = ((val_str == "true") || (val_str == "1"));
}

template <>
void Field<std::string>::SetVal(DataElem& val, const std::string& val_str) {
  val = val_str;
}
