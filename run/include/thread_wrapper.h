#ifndef RUN_THREADWRAPPER_H_
#define RUN_THREADWRAPPER_H_

#include <pthread.h>
#include <signal.h>

#include "run_base.h"

class ThreadWrapper : public RunBase {
 public:
  ThreadWrapper();
  ~ThreadWrapper();

  int SetThreadAttr(int flag);  // flag 无效
  virtual int Run(void* arg = nullptr);
  virtual int Stop(void* arg = nullptr);

 private:
  void ClearAttr() { pthread_attr_destroy(&attr_); }
  static void* Entry(void* arg);
  static void Sigaction(int signo, siginfo_t* info, void* context);

 private:
  pthread_t pthread_;
  pthread_attr_t attr_;
};

#endif  // RUN_THREADWRAPPER_H_
