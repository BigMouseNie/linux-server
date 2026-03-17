#include "thread_pool.h"

ThreadPool::ThreadPool(int thrd_num) {
  if (thrd_num <= 0) thrd_num = 1;
  for (int i = 0; i < thrd_num; ++i) {
    workers_.emplace_back(&ThreadPool::Work, this);
  }
}

ThreadPool::~ThreadPool() {
  que_.Release();
  for (auto& thrd : workers_) {
    if (thrd.joinable()) thrd.join();
  }
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
