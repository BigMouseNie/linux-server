#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "blocking_queue.h"

TEST(BlockingQueueTest, TrivialType) {
  std::vector<std::thread> producer;
  std::vector<std::thread> consumer;
  BlockingQueue<int> que;
  const int maxVal = 1000000;
  const int producer_num = 2;
  const int consumer_num = 2;
  std::atomic<int> producer_remain = maxVal;
  std::atomic<long long> consumer_sum = 0;
  for (int i = 0; i < consumer_num; ++i) {
    consumer.emplace_back(std::thread([&]() {
      int elem = 0;
      while (que.Pop(elem)) {
        consumer_sum.fetch_add(elem);
      }
    }));
  }

  for (int i = 0; i < producer_num; ++i) {
    producer.emplace_back(std::thread([&]() {
      while (true) {
        int val = producer_remain.fetch_sub(1);
        if (val <= 0) {
          break;
        }
        que.Push(val);
      }
    }));
  }

  for (auto& t : producer) {
    if (t.joinable()) t.join();
  }

  que.Release(false);

  for (auto& t : consumer) {
    if (t.joinable()) t.join();
  }

  long long result = 0;
  for (int i = 0; i <= maxVal; ++i) result += i;
  EXPECT_EQ(producer_remain.load(), -2);
  EXPECT_EQ(consumer_sum.load(), result);
  EXPECT_TRUE(que.Empty() == true);
}

class IntCls {
 public:
  IntCls() : val(new int(0)) {}
  explicit IntCls(int v) : val(new int(v)) {}
  IntCls(const IntCls& other) : val(new int(other.GetVal())) {}
  IntCls(IntCls&& other) noexcept : val(other.val) {
    other.val = nullptr;
    move_cnt.fetch_add(1);
  }

  IntCls& operator=(const IntCls& other) {
    if (this != &other) {
      *val = other.GetVal();
    }
    return *this;
  }

  IntCls& operator=(IntCls&& other) noexcept {
    move_cnt.fetch_add(1);
    if (this != &other) {
      delete val;
      val = other.val;
      other.val = nullptr;
    }
    return *this;
  }

  ~IntCls() {
    if (val) {
      delete val;
      val = nullptr;
    }
  }

  int GetVal() const { return *val; }
  static int GetMoveCnt() { return move_cnt.load(); }

 private:
  int* val;
  static std::atomic<int> move_cnt;
};

std::atomic<int> IntCls::move_cnt = 0;

TEST(BlockingQueueTest, NontrivialType01) {
  std::vector<std::thread> producer;
  std::vector<std::thread> consumer;
  BlockingQueue<IntCls> que;
  const int maxVal = 1000000;
  const int producer_num = 2;
  const int consumer_num = 2;
  std::atomic<int> producer_remain = maxVal;
  std::atomic<long long> consumer_sum = 0;
  for (int i = 0; i < consumer_num; ++i) {
    consumer.emplace_back(std::thread([&]() {
      IntCls int_cls;
      while (que.Pop(int_cls)) {
        consumer_sum.fetch_add(int_cls.GetVal());
      }
    }));
  }

  for (int i = 0; i < producer_num; ++i) {
    producer.emplace_back(std::thread([&]() {
      while (true) {
        int val = producer_remain.fetch_sub(1);
        if (val <= 0) {
          break;
        }
        IntCls int_cls(val);
        que.Push(std::move(int_cls));
      }
    }));
  }

  for (auto& t : producer) {
    if (t.joinable()) t.join();
  }

  que.Release(false);

  for (auto& t : consumer) {
    if (t.joinable()) t.join();
  }

  long long result = 0;
  for (int i = 0; i <= maxVal; ++i) result += i;
  EXPECT_EQ(producer_remain.load(), -2);
  EXPECT_EQ(consumer_sum.load(), result);
  EXPECT_EQ(IntCls::GetMoveCnt(), maxVal * 2);
  EXPECT_TRUE(que.Empty() == true);
}

class IntClsNoMove {
 public:
  IntClsNoMove() : val(new int(0)) {}
  explicit IntClsNoMove(int v) : val(new int(v)) {}
  IntClsNoMove(const IntClsNoMove& other) : val(new int(other.GetVal())) {}
  IntClsNoMove(IntClsNoMove&& other) = delete;

  IntClsNoMove& operator=(const IntClsNoMove& other) {
    if (this != &other) {
      *val = other.GetVal();
    }
    return *this;
  }
  IntClsNoMove& operator=(IntClsNoMove&& other) = delete;

  ~IntClsNoMove() {
    if (val) {
      delete val;
      val = nullptr;
    }
  }

  int GetVal() const { return *val; }

 private:
  int* val;
};

TEST(BlockingQueueTest, NontrivialType02) {
  std::vector<std::thread> producer;
  std::vector<std::thread> consumer;
  BlockingQueue<IntClsNoMove> que;
  const int maxVal = 1000000;
  const int producer_num = 2;
  const int consumer_num = 2;
  std::atomic<int> producer_remain = maxVal;
  std::atomic<long long> consumer_sum = 0;
  for (int i = 0; i < consumer_num; ++i) {
    consumer.emplace_back(std::thread([&]() {
      IntClsNoMove int_cls_nomv;
      while (que.Pop(int_cls_nomv)) {
        consumer_sum.fetch_add(int_cls_nomv.GetVal());
      }
    }));
  }

  for (int i = 0; i < producer_num; ++i) {
    producer.emplace_back(std::thread([&]() {
      while (true) {
        int val = producer_remain.fetch_sub(1);
        if (val <= 0) {
          break;
        }
        IntClsNoMove int_cls_nomv(val);
        que.Push(int_cls_nomv);
      }
    }));
  }

  for (auto& t : producer) {
    if (t.joinable()) t.join();
  }

  que.Release(false);

  for (auto& t : consumer) {
    if (t.joinable()) t.join();
  }

  long long result = 0;
  for (int i = 0; i <= maxVal; ++i) result += i;
  EXPECT_EQ(producer_remain.load(), -2);
  EXPECT_EQ(consumer_sum.load(), result);
  EXPECT_TRUE(que.Empty() == true);
}

// Size() tests

TEST(BlockingQueueTest, SizeEmpty) {
  BlockingQueue<int> que;
  EXPECT_EQ(que.Size(), 0);
  EXPECT_TRUE(que.Empty());
}

TEST(BlockingQueueTest, SizeAfterPush) {
  BlockingQueue<int> que;
  que.Push(1);
  EXPECT_EQ(que.Size(), 1);

  que.Push(2);
  que.Push(3);
  EXPECT_EQ(que.Size(), 3);
}

TEST(BlockingQueueTest, SizeAfterPop) {
  BlockingQueue<int> que;
  que.Push(1);
  que.Push(2);
  que.Push(3);
  EXPECT_EQ(que.Size(), 3);

  int val;
  que.Pop(val);
  EXPECT_EQ(que.Size(), 2);

  que.Pop(val);
  que.Pop(val);
  EXPECT_EQ(que.Size(), 0);
}

TEST(BlockingQueueTest, SizeMultiThreaded) {
  BlockingQueue<int> que;
  const int push_count = 1000;
  std::atomic<int> push_done{0};

  // Producer thread
  std::thread producer([&]() {
    for (int i = 0; i < push_count; ++i) {
      que.Push(i);
    }
    push_done = 1;
  });

  // Wait for producer to finish
  producer.join();

  // Size should be push_count
  EXPECT_EQ(que.Size(), push_count);

  que.Release();
}

TEST(BlockingQueueTest, SizeWithConcurrentAccess) {
  BlockingQueue<int> que;
  const int push_count = 10000;
  std::atomic<bool> done{false};

  // Producer thread
  std::thread producer([&]() {
    for (int i = 0; i < push_count; ++i) {
      que.Push(i);
    }
    done = true;
  });

  // Consumer thread - also checks size
  std::thread consumer([&]() {
    int val;
    while (!done || que.Size() > 0) {
      que.Pop(val);
    }
  });

  producer.join();
  que.Release();
  consumer.join();

  EXPECT_EQ(que.Size(), 0);
}

TEST(BlockingQueueTest, SizeAfterRelease) {
  BlockingQueue<int> que;
  que.Push(1);
  que.Push(2);
  que.Push(3);
  EXPECT_EQ(que.Size(), 3);

  que.Release(true);  // Release with clear
  EXPECT_EQ(que.Size(), 0);
  EXPECT_TRUE(que.Empty());
}

TEST(BlockingQueueTest, SizeWithMovePush) {
  BlockingQueue<std::string> que;
  std::string str1 = "hello";
  std::string str2 = "world";

  que.Push(std::move(str1));
  que.Push(std::move(str2));

  EXPECT_EQ(que.Size(), 2);
}
