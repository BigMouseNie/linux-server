#include "thread_pool.h"

ThreadPool::ThreadPool::~ThreadPool() {
  que_.Release();
  for (auto& thrd : workers_) {
    if (thrd.joinable()) thrd.join();
  }
}

int ThreadPool::Start(int thrd_num) {
  for (int i = 0; i < thrd_num; ++i) {
    workers_.emplace_back(&ThreadPool::Work, this);
  }
  return 0;
}

void ThreadPool::Work() {
  while (true) {
    Task task;
    if (!que_.Pop(task)) {
      break;
    }
    task();
  }
}
