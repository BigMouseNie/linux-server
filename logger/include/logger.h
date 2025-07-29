#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <unistd.h>

#include "acceptor.h"
#include "epoller.h"
#include "socket_wrapper.h"
#include "thread_wrapper.h"

class Logger {
 public:
  Logger() : sock_serv_(-1) {};
  ~Logger() { sock_serv_.SetManualMgnt(false); }

  int Create(SocketWrapper&& sock_serv, char* save_path, bool is_et);

 private:
  int Worker();

 private:
  SocketWrapper sock_serv_;
  Epoller epoller_;
  Acceptor acceptor_;
  ThreadWrapper worker_;
  char path_[256];
};

#endif  // LOGGER_LOGGER_H_
