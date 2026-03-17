#ifndef CONTAINER_BLOCKINGQUEUE_H_
#define CONTAINER_BLOCKINGQUEUE_H_

#include <stddef.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue() : release_(false) {}
  ~BlockingQueue() { Release(); }

  BlockingQueue(const BlockingQueue&) = delete;
  BlockingQueue(BlockingQueue&&) = delete;
  BlockingQueue& operator=(const BlockingQueue&) = delete;
  BlockingQueue& operator=(BlockingQueue&&) = delete;

  bool Pop(T& outElem);
  bool Push(const T& elem);
  bool Push(T&& elem);
  bool Empty();
  size_t Size();
  void Blocking();
  void Release(bool is_clear = false);

 private:
  size_t SwapQueue();
  void Clear();

 private:
  std::mutex pop_mtx_;
  std::mutex push_mtx_;
  std::queue<T> pop_que_;
  std::queue<T> push_que_;
  std::condition_variable no_empty_;
  std::atomic<bool> release_;
};

template <typename T>
bool BlockingQueue<T>::Pop(T& out_elem) {
  std::lock_guard<std::mutex> pop_lock(pop_mtx_);
  if (pop_que_.empty() && SwapQueue() == 0) {
    return false;
  }

  auto& front = pop_que_.front();
  if constexpr (std::is_move_assignable_v<T>) {
    out_elem = std::move(front);
  } else if constexpr (std::is_copy_assignable_v<T>) {
    out_elem = front;
  } else {
    static_assert(std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>,
                  "Type must be move- or copy-assignable to Pop()");
  }

  pop_que_.pop();
  return true;
}

template <typename T>
bool BlockingQueue<T>::Push(const T& elem) {
  {
    std::lock_guard<std::mutex> push_lock(push_mtx_);
    if (release_) {
      return false;
    }
    push_que_.push(elem);
  }
  no_empty_.notify_one();
  return true;
}

template <typename T>
bool BlockingQueue<T>::Push(T&& elem) {
  {
    std::lock_guard<std::mutex> push_lock(push_mtx_);
    if (release_) {
      return false;
    }
    push_que_.push(std::move(elem));
  }
  no_empty_.notify_one();
  return true;
}

template <typename T>
bool BlockingQueue<T>::Empty() {
  std::lock_guard<std::mutex> pop_lock(pop_mtx_);
  std::lock_guard<std::mutex> push_lock(push_mtx_);
  return pop_que_.empty() && push_que_.empty();
}

template <typename T>
size_t BlockingQueue<T>::Size() {
  std::lock_guard<std::mutex> pop_lock(pop_mtx_);
  std::lock_guard<std::mutex> push_lock(push_mtx_);
  return pop_que_.size() + push_que_.size();
}

// 多线程只可能有一个线程进入，并且持有pop_mtx_(Pop操作)
template <typename T>
size_t BlockingQueue<T>::SwapQueue() {
  std::unique_lock<std::mutex> push_lock(push_mtx_);
  no_empty_.wait(push_lock,
                 [this]() { return !push_que_.empty() || release_; });
  std::swap(pop_que_, push_que_);
  return pop_que_.size();
}

template <typename T>
void BlockingQueue<T>::Blocking() {
  release_ = false;
}

template <typename T>
void BlockingQueue<T>::Release(bool is_clear) {
  bool expected = false;
  if (release_.compare_exchange_strong(expected, true)) {
    no_empty_.notify_all();
  }
  if (is_clear) Clear();
}

template <typename T>
void BlockingQueue<T>::Clear() {
  std::lock_guard<std::mutex> pop_lock(pop_mtx_);
  std::lock_guard<std::mutex> push_lock(push_mtx_);
  std::queue<T> empty1;
  std::queue<T> empty2;
  std::swap(pop_que_, empty1);
  std::swap(push_que_, empty2);
}

#endif  // CONTAINER_BLOCKINGQUEUE_H_
