#ifndef POOL_THREADPOOL_H_
#define POOL_THREADPOOL_H_

#include <functional>
#include <thread>
#include <vector>

#include "blocking_queue.h"

class ThreadPool {
  using Task = std::function<void()>;
  using TaskQue = BlockingQueue<Task>;

 public:
  explicit ThreadPool(int thrd_num = 1);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <typename Func, typename... Args>
  bool AddTask(Func&& func, Args&&... args) {
    auto task =
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    return que_.Push(std::move(task));
  }

 private:
  void Work();

 private:
  TaskQue que_;
  std::vector<std::thread> workers_;
};

#endif  // POOL_THREADPOOL_H_
