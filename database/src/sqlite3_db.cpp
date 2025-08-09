#include "sqlite3_db.h"

#include <sqlite3.h>

int Sqlite3DB::connect(const std::string& file) {
  int ret = sqlite3_open(file.c_str(), &db_);
  if (ret != SQLITE_OK || !db_) {
    return -1;
  }
  return 0;
}

int Sqlite3DB::Exec(const std::string& sql) {
  char* err_msg = nullptr;
  int ret = sqlite3_exec(db_, sql.c_str(), &Sqlite3DB::ExecCallback, nullptr,
                         &err_msg);
  if (ret != SQLITE_OK) {
    if (err_msg) {
      // log
      sqlite3_free(err_msg);
    }
    return -1;
  }
  return 0;
}

int Sqlite3DB::Exec(const std::string& sql, Table& result) {
  char* err_msg = nullptr;
  int ret = sqlite3_exec(db_, sql.c_str(), &Sqlite3DB::ExecCallback, &result,
                         &err_msg);
  if (ret != SQLITE_OK) {
    if (err_msg) {
      // log
      sqlite3_free(err_msg);
    }
    return -1;
  }
  return 0;
}

int Sqlite3DB::StartTransaction() {
  char* err_msg = nullptr;
  int ret = sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, &err_msg);
  if (ret != SQLITE_OK) {
    sqlite3_free(err_msg);
    return -1;
  }
  return 0;
}

int Sqlite3DB::CommitTransaction() {
  char* err_msg = nullptr;
  int ret = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err_msg);
  if (ret != SQLITE_OK) {
    sqlite3_free(err_msg);
    return -1;
  }
  return 0;
}

int Sqlite3DB::RollbackTransaction() {
  char* err_msg = nullptr;
  int ret = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err_msg);
  if (ret != SQLITE_OK) {
    sqlite3_free(err_msg);
    return -1;
  }
  return 0;
}

void Sqlite3DB::Close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

int Sqlite3DB::ExecCallback(void* table, int num, char** vals, char** fields) {
  if (!table) {
    return -1;
  }
  Table* table_obj = static_cast<Table*>(table);
  Row row(*table_obj);
  for (int i = 0; i < num; ++i) {
    row.PushDataElem(fields[i], vals[i]);
  }
  table_obj->PushRow(std::move(row));
  return 0;
}
