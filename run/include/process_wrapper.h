#ifndef RUN_PROCESSWRAPPER_H_
#define RUN_PROCESSWRAPPER_H_

#include "run_base.h"
 
class ProcessWrapper : public RunBase {
 public:
  ProcessWrapper() = default;
  ~ProcessWrapper() = default;

  virtual int Run(void* arg = nullptr);

  int ReadFdFromPipe(int& fd);
  int WriteFdToPipe(int fd);
  static int Daemonize();

 private:
  pid_t pid_;
  int pipe_[2];
};

#endif  // RUN_PROCESSWRAPPER_H_
