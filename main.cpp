#include <iostream>

#include "logger.h"
#include "process_wrapper.h"
#include "socket_creator.h"
#include "thread_wrapper.h"
#include "thread_pool.h"

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
  std::cout << "main proc pid : " << getpid() << std::endl;
  sleep(1); // wait LoggerServer start

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
  return 0;
}
