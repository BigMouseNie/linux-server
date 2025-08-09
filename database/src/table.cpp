#include "table.h"

#include "field.h"

Row::Row(const Table& table) : table_(table) {
  row_data_.resize(table_.fields_.size());
}

const DataElem& Row::GetDataElem(const std::string& field_aliases, bool& res) const {
  auto it = table_.fields_map_.find(field_aliases);
  if (it == table_.fields_map_.end()) {
    res = false;
    return table_.null_val_;
  }
  res = true;
  return row_data_[it->second];
}

void Row::PushDataElem(const std::string& field_aliases, const DataElem& val) {
  auto it = table_.fields_map_.find(field_aliases);
  if (it != table_.fields_map_.end()) {
    row_data_[it->second] = val;
  }
}
void Row::PushDataElem(const std::string& field_aliases, DataElem&& val) {
  auto it = table_.fields_map_.find(field_aliases);
  if (it != table_.fields_map_.end()) {
    row_data_[it->second] = std::move(val);
  }
}
void Row::PushDataElem(const std::string& field_aliases, const char* val_str) {
  auto it = table_.fields_map_.find(field_aliases);
  if (it != table_.fields_map_.end()) {
    table_.fields_[it->second]->SetVal(row_data_[it->second], val_str);
  }
}
void Row::PushDataElem(const std::string& field_aliases, std::string& val_str) {
  return PushDataElem(field_aliases, val_str.c_str());
}

int Table::AddField(const FieldBase& field) {
  std::shared_ptr<FieldBase> field_ptr(field.DeepCopy());
  fields_.push_back(field_ptr);
  fields_map_[field_ptr->GetAliases()] = fields_.size() - 1;
  return 0;
}

int Table::PushRow(const Row& row) {
  data_.push_back(row);
  return 0;
}
int Table::PushRow(Row&& row) {
  data_.push_back(std::move(row));
  return 0;
}

void Table::ClearFields() {
  fields_.clear();
  fields_map_.clear();
}
