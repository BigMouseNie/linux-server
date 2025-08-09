#ifndef DATABASE_DBTYPE_H_
#define DATABASE_DBTYPE_H_

#include <memory>
#include <variant>
#include <vector>

class FieldBase;

using DataElem = std::variant<std::monostate, int, int64_t, double, std::string,
                              bool, std::vector<uint8_t>>;
using Fields = std::vector<std::shared_ptr<FieldBase>>;

#endif  // DATABASE_DBTYPE_H_
