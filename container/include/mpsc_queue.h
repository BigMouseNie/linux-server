#ifndef CONTAINER_MPSCQUEUE_H_
#define CONTAINER_MPSCQUEUE_H_

#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

template <typename T>
struct MPSCNode {
  std::atomic<MPSCNode*> next{nullptr};
  // 保证不调用构造函数; 保证同T是同样的方式内存对齐
  alignas(T) unsigned char storage[sizeof(T)];

  MPSCNode() = default;  // 哨兵节点使用(无构造)

  template <typename U>
  MPSCNode(U&& val) {  // 数据节点使用
    // placement new, 在已分配内存上构造对象(无论平凡性质，由alignas保证)
    new (storage) T(std::forward<U>(val));  // 需要手动调用~T()
  }

  T* Data() { return reinterpret_cast<T*>(storage); }
  const T* Data() const { return reinterpret_cast<const T*>(storage); }
};

template <typename T>
class MPSCQueue {
 public:
  MPSCQueue();
  ~MPSCQueue();

  MPSCQueue(const MPSCQueue&) = delete;
  MPSCQueue(MPSCQueue&&) = delete;
  MPSCQueue& operator=(const MPSCQueue&) = delete;
  MPSCQueue& operator=(MPSCQueue&&) = delete;

  bool Push(const T& elem);
  bool Push(T&& elem);
  bool Pop(T& outElem);
  bool Empty() const;
  size_t Size() const;

 private:
  std::atomic<MPSCNode<T>*> head_;
  std::atomic<MPSCNode<T>*> tail_;
};

template <typename T>
MPSCQueue<T>::MPSCQueue() : head_(new MPSCNode<T>()) {
  tail_.store(head_, std::memory_order_relaxed);
}

template <typename T>
MPSCQueue<T>::~MPSCQueue() {
  T elem;
  while (Pop(elem)) {
  }
  delete head_;
}

template <typename T>
bool MPSCQueue<T>::Push(const T& elem) {
  auto* node = new MPSCNode<T>(elem);
  auto* prev = tail_.exchange(node, std::memory_order_acq_rel);
  prev->next.store(node, std::memory_order_release);
  return true;
}

template <typename T>
bool MPSCQueue<T>::Push(T&& elem) {
  auto* node = new MPSCNode<T>(std::move(elem));
  auto* prev = tail_.exchange(node, std::memory_order_acq_rel);
  prev->next.store(node, std::memory_order_release);
  return true;
}

template <typename T>
bool MPSCQueue<T>::Pop(T& outElem) {
  auto* head = head_.load(std::memory_order_relaxed);
  auto* next = head->next.load(std::memory_order_acquire);
  if (!next) return false;
  outElem = std::move(*next->Data());
  next->Data()->~T();
  head_.store(next, std::memory_order_relaxed);
  delete head;
  return true;
}

template <typename T>
bool MPSCQueue<T>::Empty() const {
  return head_.load(std::memory_order_relaxed)
             ->next.load(std::memory_order_acquire) == nullptr;
}

template <typename T>
size_t MPSCQueue<T>::Size() const {
  size_t count = 0;
  auto* node = head_.load(std::memory_order_relaxed)
                   ->next.load(std::memory_order_acquire);
  while (node) {
    ++count;
    node = node->next.load(std::memory_order_relaxed);
  }
  return count;
}

#endif  // CONTAINER_MPSCQUEUE_H_
