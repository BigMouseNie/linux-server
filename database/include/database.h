#ifndef DATABASE_DATABASE_H_
#define DATABASE_DATABASE_H_

#include <string>

#include "table.h"

class Database {
 public:
  Database() = default;
  ~Database() = default;

  virtual int connect() = 0;
  virtual int Exec(const std::string& sql) = 0;
  virtual int Exec(const std::string& sql, Table& result) = 0;
  virtual int StartTransaction() = 0;
  virtual int CommitTransaction() = 0;
  virtual int RollbackTransaction() = 0;
  virtual void Close() = 0;

 private:
};

#endif  // DATABASE_DATABASE_H_
