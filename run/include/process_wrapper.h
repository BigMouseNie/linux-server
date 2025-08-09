#ifndef RUN_PROCESSWRAPPER_H_
#define RUN_PROCESSWRAPPER_H_

#include "run_base.h"

class ProcessWrapper : public RunBase {
 public:
  ProcessWrapper() : pid_(0), fd_(-1) {};
  ~ProcessWrapper() = default;

  virtual int Run(void* arg = nullptr);

  int ReadFdFromPipe(int& fd, void* info = nullptr, int* info_len = nullptr);
  int WriteFdToPipe(int fd, void* info = nullptr, int info_len = 0);

  static int Daemonize();

 private:
  pid_t pid_;
  int pipe_[2];
  int fd_;
};

#endif  // RUN_PROCESSWRAPPER_H_
