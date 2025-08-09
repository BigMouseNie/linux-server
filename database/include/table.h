#ifndef DATABASE_TABLE_H_
#define DATABASE_TABLE_H_

#include <string>
#include <unordered_map>

#include "db_type.h"

class Table;
class Row {
  friend class Table;

 public:
  Row(const Table& table);
  ~Row() = default;
  const DataElem& GetDataElem(const std::string& field_aliases, bool& res) const;
  void PushDataElem(const std::string& field_aliases, const DataElem& val);
  void PushDataElem(const std::string& field_aliases, DataElem&& val);
  void PushDataElem(const std::string& field_aliases, const char* val_str);
  void PushDataElem(const std::string& field_aliases, std::string& val_str);

 private:
  const Table& table_;
  std::vector<DataElem> row_data_;
};

class Table {
  friend class Row;

 public:
  Table() : null_val_(std::monostate{}) {}
  ~Table() = default;

  std::string GetName() { return name_; }
  void SetName(const std::string& name) { name_ = name; }
  void ClearData() { data_.clear(); }
  void ClearAll() {
    ClearData();
    ClearFields();
  }
  int AddField(const FieldBase& field);
  int PushRow(const Row& row);
  int PushRow(Row&& row);
  int Count() {return data_.size();}
  const Row* GetRow(size_t idx) const {
    if (idx >= data_.size()) { 
      return nullptr;
    }
    return &(data_[idx]);
  }

 private:
  void ClearFields();

 private:
  std::string name_;
  Fields fields_;
  std::unordered_map<std::string, int> fields_map_;
  std::vector<Row> data_;
  DataElem null_val_;
};

#endif  // DATABASE_TABLE_H_
