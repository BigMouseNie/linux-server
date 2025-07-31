#include <iostream>

#include "logger.h"
#include "process_wrapper.h"
#include "socket_creator.h"
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

int main() {
  ProcessWrapper proc1;
  proc1.Init(CreateLoggerServer, &proc1);
  int pid = proc1.Run();
  std::cout << "sub proc pid : " << pid << std::endl;

  sleep(1); // wait LoggerServer start

  ProcessWrapper proc2;
  proc2.Init(LogTest, 1);
  proc2.Run();

  ProcessWrapper proc3;
  proc3.Init(LogTest, 2);
  proc3.Run();

  ThreadWrapper thrd1;
  thrd1.Init(LogTest, 3);
  thrd1.Run();

  ThreadWrapper thrd2;
  thrd2.Init(LogTest, 4);
  thrd2.Run();

  ThreadWrapper thrd3;
  thrd3.Init(LogTest, 5);
  thrd3.Run();

  LogTest(6);

  sleep(1);
  std::cout << "main end" << std::endl;
  return 0;
}
