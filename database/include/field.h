#ifndef DATABASE_FIELD_H_
#define DATABASE_FIELD_H_

#include <string>

#include "db_type.h"

class FieldBase {
 public:
  FieldBase() = default;
  ~FieldBase() = default;

  virtual std::string GetName() const = 0;
  virtual std::string GetAliases() const = 0;
  virtual void SetName(const std::string& name) = 0;
  virtual void SetAliases(const std::string& aliases) = 0;
  virtual void SetAllName(const std::string& name) = 0;
  virtual void SetVal(DataElem& val, const std::string& val_str) = 0;
  virtual FieldBase* DeepCopy() const = 0;
};

template <typename T>
class Field : public FieldBase {
 public:
  Field() = default;
  ~Field() = default;
  Field(const Field& other)
      : name_(other.name_),
        aliases_(other.aliases_),
        attr_(other.attr_),
        def_val_(other.def_val_) {}

  Field& operator=(const Field& other) {
    if (this == &other) {
      return *this;
    }
    name_ = other.name_;
    aliases_ = other.aliases_;
    attr_ = other.attr_;
    def_val_ = other.def_val_;
    return *this;
  }

  std::string GetName() const { return name_; }
  std::string GetAliases() const { return aliases_; }
  void SetName(const std::string& name) { name_ = name; }
  void SetAliases(const std::string& aliases) { aliases_ = aliases; }
  void SetAllName(const std::string& name) {
    name_ = name;
    aliases_ = name;
  }

  void SetVal(DataElem& val, const std::string& val_str);
  virtual FieldBase* DeepCopy() const {
    FieldBase* field = new Field(*this);
    return field;
  }

 private:
  std::string name_;
  std::string aliases_;
  int attr_;
  T def_val_;
};

template <>
void Field<int>::SetVal(DataElem& val, const std::string& val_str);

template <>
void Field<double>::SetVal(DataElem& val, const std::string& val_str);

template <>
void Field<bool>::SetVal(DataElem& val, const std::string& val_str);

template <>
void Field<std::string>::SetVal(DataElem& val, const std::string& val_str);

#endif  // DATABASE_FIELD_H_
