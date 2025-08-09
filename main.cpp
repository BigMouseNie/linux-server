#include <iostream>

#include "crypto_wrap.h"
#include "field.h"
#include "http_parser_wrap.h"
#include "logger.h"
#include "process_wrapper.h"
#include "socket_creator.h"
#include "sqlite3_db.h"
#include "thread_pool.h"
#include "thread_wrapper.h"

int CreateLoggerServer(ProcessWrapper* proc) {
  std::cout << "start create logger server" << std::endl;
  int ret = Logger::Instance().Create();
  std::cout << "logger create ret : " << ret << std::endl;
  int fd = -1;
  while (true) {
    proc->ReadFdFromPipe(fd);
    if (fd < 0) {
      break;
    }
  }

  return ret;
}

void LogTest(int x) {
  LOG_DEBUG("This is test(DEBU) ====> %d", x);
  LOG_INFO("This is test(INFO) ====> %d", x);
  LOG_WARN("This is test(WARN) ====> %d", x);
  LOG_ERROR("This is test(ERRO) ====> %d", x);
  LOG_FATAL("This is test(FATA) ====> %d", x);
}

void ModLogTest() {
  ProcessWrapper proc1;
  proc1.Init(CreateLoggerServer, &proc1);
  int pid = proc1.Run();
  std::cout << "sub proc pid : " << pid << std::endl;
  std::cout << "main proc pid : " << getpid() << std::endl;
  sleep(1);  // wait LoggerServer start

  ThreadPool* thrd_pool = new ThreadPool;
  thrd_pool->Start(4);

  thrd_pool->AddTask(LogTest, 1);
  thrd_pool->AddTask(LogTest, 2);
  thrd_pool->AddTask(LogTest, 3);
  thrd_pool->AddTask(LogTest, 4);
  thrd_pool->AddTask(LogTest, 5);
  thrd_pool->AddTask(LogTest, 6);
  LogTest(7);

  sleep(1);
  proc1.WriteFdToPipe(-1);
  sleep(1);
  std::cout << "main end: pid : " << getpid() << std::endl;
}

int HttpTest() {
    const char* http_request =
      "GET /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "\r\n";

  HttpParserWrap hp_wrap;
  int ret = hp_wrap.Parser(http_request);
  if (ret < 0) {
    std::cout << "http parser failed" << std::endl;
    std::cout << hp_wrap.GetError() << std::endl;
    return -1;
  }
  std::cout << "http parser success" << std::endl;
  std::string str;
  int t;
  hp_wrap.Body(str);
  std::cout << str << std::endl;
  hp_wrap.Status(t);
  std::cout << t << std::endl;
  hp_wrap.Method(t);
  std::cout << t << std::endl;
  hp_wrap.Url(str);
  std::cout << "Url : " << str << std::endl;
  return 0;
}

void UrlTest() {
  std::string uir_str("https://example.com/search/info?q=me&lang=en#top");
  std::cout << "start" << std::endl;
  UrlObject url_obj(uir_str);
  std::cout << "end" << std::endl;
  if (url_obj.IsValid()) {
    std::cout << "Scheme : " << url_obj.Scheme() << std::endl;
    std::cout << "Host : " << url_obj.Host() << std::endl;
    std::cout << "Path : " << url_obj.Path() << std::endl;
    std::cout << "Query : " << url_obj.Query() << std::endl;
    std::cout << "q : " << url_obj["q"].second << std::endl;
    std::cout << "lang : " << url_obj["lang"].second << std::endl;
    std::cout << "null : " << url_obj["null"].second << std::endl;
  } else {
    std::cout << url_obj.GetErrorStr() << std::endl;
  }
}

int Sqlite3Test() {
  Sqlite3DB db;
  int ret = db.connect("test.db");
  if (ret < 0) {
    std::cout << "connect failed" << std::endl;
    return -1;
  }
  std::cout << "connect success" << std::endl;

  const char* sql = R"sql(
    CREATE TABLE IF NOT EXISTS users (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      age INTEGER,
      email TEXT UNIQUE
    );
  )sql";
  ret = db.Exec(sql);
  if (ret < 0) {
    std::cout << "create failed" << std::endl;
  } else {
    std::cout << "create success" << std::endl;
  }

  Table user_res;
  Field<int> user_id;
  user_id.SetAllName("id");

  Field<std::string> user_name;
  user_name.SetAllName("name");

  Field<int> user_age;
  user_age.SetAllName("age");

  Field<std::string> user_email;
  user_email.SetAllName("email");

  user_res.AddField(user_id);
  user_res.AddField(user_name);
  user_res.AddField(user_age);
  user_res.AddField(user_email);

  // std::string row1(
  //     "INSERT INTO users (name, age, email) VALUES ('Alice', 25, "
  //     "'alice@example.com');");
  // std::string row2(
  //     "INSERT INTO users (name, age, email) VALUES ('Bob', 30, "
  //     "'bob@example.com');");
  // std::string row3(
  //     "INSERT INTO users (name, age, email) VALUES ('Charlie', 28, "
  //     "'charlie@example.com');");
  // ret = db.Exec(row1);
  // if (ret < 0) {
  //   std::cout << "insert failed" << std::endl;
  //   return -1;
  // }

  // ret = db.Exec(row2);
  // if (ret < 0) {
  //   std::cout << "insert failed" << std::endl;
  //   return -1;
  // }

  // ret = db.Exec(row3);
  // if (ret < 0) {
  //   std::cout << "insert failed" << std::endl;
  //   return -1;
  // }

  const char* select_sql = "SELECT * FROM users;";
  ret = db.Exec(select_sql, user_res);
  if (ret < 0) {
    std::cout << "select failed" << std::endl;
    return -1;
  }

  for (int i = 0; i < user_res.Count(); ++i) {
    const Row* row = user_res.GetRow(i);
    if (!row) {
      continue;
    }
    bool res = false;
    const DataElem& elem_id = row->GetDataElem("id", res);
    if (res) {
      std::cout << std::get<int>(elem_id) << std::endl;
    }

    const DataElem& elem_name = row->GetDataElem("name", res);
    if (res) {
      std::cout << std::get<std::string>(elem_name) << std::endl;
    }
  }

  return 0;
}

int main() {
  //  pass
  return 0;
}
