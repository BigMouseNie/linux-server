#include "thread_wrapper.h"

#include <errno.h>
#include <string.h>

ThreadWrapper::ThreadWrapper() : pthread_(0) { attr_ = {0}; }
ThreadWrapper::~ThreadWrapper() {
  if (state_ == RunState::kRunning || state_ == RunState::kPaused) {
    Stop();
  }
  if (pthread_ != 0) {
    pthread_join(pthread_, nullptr);
  }
  ClearAttr();
}

int ThreadWrapper::SetThreadAttr(int flag) {
  ClearAttr();
  int ret = pthread_attr_init(&attr_);
  if (ret != 0) return -1;

  ret = pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);
  if (ret != 0) return -2;
  return 0;
}

int ThreadWrapper::Run(void* arg) {
  if (state_ != RunState::kInitialized) {
    return -1;
  }
  // set attr
  int ret = SetThreadAttr(0);
  if (ret != 0) return -2;

  ret = pthread_create(&pthread_, &attr_, &Entry, this);
  if (ret != 0) return -3;

  state_ = RunState::kRunning;
  return 0;
}

int ThreadWrapper::Stop(void* arg) {
  if (state_ == RunState::kStopped) return 0;
  if (state_ != RunState::kRunning && state_ != RunState::kPaused) {
    return -1;
  }
  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100 * 1000 * 1000;  // 100ms
  int ret = pthread_timedjoin_np(pthread_, nullptr, &ts);
  if (ret == ETIMEDOUT) {
    pthread_kill(pthread_, SIGUSR2);
    pthread_join(pthread_, nullptr);
  }
  state_ = RunState::kStopped;
  pthread_ = 0;
  return 0;
}

void* ThreadWrapper::Entry(void* arg) {
  ThreadWrapper* thiz = static_cast<ThreadWrapper*>(arg);
  struct sigaction act = {0};
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &ThreadWrapper::Sigaction;
  // sigaction(SIGUSR1, &act, nullptr); // pause
  sigaction(SIGUSR2, &act, nullptr);  // stop
  thiz->entry_();
  thiz->state_ = RunState::kStopped;
  pthread_exit(nullptr);
}

void ThreadWrapper::Sigaction(int signo, siginfo_t* info, void* context) {
  if (signo == SIGUSR2) {  // stop
    pthread_exit(nullptr);
  }
}
