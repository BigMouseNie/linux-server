#ifndef DATABASE_SQLITE3DB_H_
#define DATABASE_SQLITE3DB_H_

#include "database.h"

class sqlite3;
class Sqlite3DB : public Database {
 public:
  Sqlite3DB() : db_(nullptr) {}
  ~Sqlite3DB() { Close(); }

  int connect() { return -1; };
  int connect(const std::string& file);
  int Exec(const std::string& sql);
  int Exec(const std::string& sql, Table& result);
  int StartTransaction();
  int CommitTransaction();
  int RollbackTransaction();
  void Close();

 private:
  static int ExecCallback(void* table, int num, char** vals, char** fields);

 private:
  sqlite3* db_;
};

#endif
