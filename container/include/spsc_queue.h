#ifndef CONTAINER_SPSCQUEUE_H_
#define CONTAINER_SPSCQUEUE_H_

#include <atomic>

template <typename T>
class SPSCQueue {
 public:
  explicit SPSCQueue(size_t cap)
      : mask_(RoundUpPow2(cap) - 1), que_(new T[mask_ + 1]) {};
  ~SPSCQueue() { delete[] que_; };
  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue(SPSCQueue&&) = delete;
  SPSCQueue& operator=(const SPSCQueue&) = delete;
  SPSCQueue& operator=(SPSCQueue&&) = delete;

  bool Pop(T& outElem);
  bool Push(const T& elem);
  bool Push(T&& elem);
  bool Empty();
  size_t Size();

 private:
  static size_t RoundUpPow2(size_t n) {
    if (n < 2) return 2;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if __SIZEOF_POINTER__ == 8
    n |= n >> 32;
#endif
    return n + 1;
  }

  size_t mask_;
  T* que_;
  std::atomic<size_t> read_idx_{0};
  std::atomic<size_t> write_idx_{0};
};

template <typename T>
bool SPSCQueue<T>::Pop(T& outElem) {
  size_t ridx_local = read_idx_.load(std::memory_order_relaxed);
  size_t widx_local = write_idx_.load(std::memory_order_acquire);
  if (ridx_local == widx_local) return false;
  outElem = std::move(que_[ridx_local & mask_]);
  read_idx_.fetch_add(1, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::Push(const T& elem) {
  size_t ridx_local = read_idx_.load(std::memory_order_acquire);
  size_t widx_local = write_idx_.load(std::memory_order_relaxed);
  if (((widx_local + 1) & mask_) == (ridx_local & mask_)) return false;
  que_[widx_local & mask_] = elem;
  write_idx_.fetch_add(1, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::Push(T&& elem) {
  size_t ridx_local = read_idx_.load(std::memory_order_acquire);
  size_t widx_local = write_idx_.load(std::memory_order_relaxed);
  if (((widx_local + 1) & mask_) == (ridx_local & mask_)) return false;
  que_[widx_local & mask_] = std::move(elem);
  write_idx_.fetch_add(1, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::Empty() {
  size_t ridx_local = read_idx_.load(std::memory_order_relaxed);
  size_t widx_local = write_idx_.load(std::memory_order_relaxed);
  return ridx_local == widx_local;
}

template <typename T>
size_t SPSCQueue<T>::Size() {
  size_t ridx_local = read_idx_.load(std::memory_order_relaxed);
  size_t widx_local = write_idx_.load(std::memory_order_relaxed);
  return (widx_local - ridx_local) & mask_;
}

#endif  // CONTAINER_SPSCQUEUE_H_
