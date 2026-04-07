#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "mpsc_queue.h"

TEST(MPSCQueueTest, EmptyAfterCreate) {
  MPSCQueue<int> q;
  EXPECT_TRUE(q.Empty());
  EXPECT_EQ(q.Size(), 0);
}

TEST(MPSCQueueTest, PushAndPop) {
  MPSCQueue<int> q;
  EXPECT_TRUE(q.Push(1));
  EXPECT_TRUE(q.Push(2));

  int val;
  EXPECT_TRUE(q.Pop(val));
  EXPECT_EQ(val, 1);
  EXPECT_TRUE(q.Pop(val));
  EXPECT_EQ(val, 2);
  EXPECT_TRUE(q.Empty());
}

TEST(MPSCQueueTest, PopOnEmpty) {
  MPSCQueue<int> q;
  int val;
  EXPECT_FALSE(q.Pop(val));
}

TEST(MPSCQueueTest, MovePush) {
  MPSCQueue<std::string> q;
  std::string s = "hello";
  EXPECT_TRUE(q.Push(std::move(s)));

  std::string out;
  EXPECT_TRUE(q.Pop(out));
  EXPECT_EQ(out, "hello");
  EXPECT_TRUE(q.Empty());
}

TEST(MPSCQueueTest, DestructorNoLeak) {
  // 验证析构函数能正确清理所有节点，不崩溃
  {
    MPSCQueue<int> q;
    for (int i = 0; i < 100; ++i) {
      q.Push(i);
    }
  }  // 析构，自动清理
}

TEST(MPSCQueueTest, PopAllUntilEmpty) {
  MPSCQueue<int> q;
  const int kCount = 50;
  for (int i = 0; i < kCount; ++i) {
    q.Push(i);
  }

  int count = 0;
  int val;
  while (q.Pop(val)) {
    ++count;
  }
  EXPECT_EQ(count, kCount);
  EXPECT_TRUE(q.Empty());
}

// ---------- MPSC 并发测试 ----------

TEST(MPSCQueueTest, MultiProducerSingleConsumer) {
  MPSCQueue<int> q;
  const int kProducers = 4;
  const int kPerProducer = 25000;
  const int kTotal = kProducers * kPerProducer;

  std::atomic<int> received{0};
  std::vector<std::thread> producers;

  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p]() {
      int base = p * kPerProducer;
      for (int i = 0; i < kPerProducer; ++i) {
        q.Push(base + i);
      }
    });
  }

  std::thread consumer([&]() {
    int val;
    for (int i = 0; i < kTotal;) {
      if (q.Pop(val)) {
        ++received;
        ++i;
      }
    }
  });

  for (auto& t : producers) {
    t.join();
  }
  consumer.join();

  EXPECT_EQ(received.load(), kTotal);
  EXPECT_TRUE(q.Empty());
}

TEST(MPSCQueueTest, ConcurrentPushAndPopNoDataLoss) {
  MPSCQueue<int> q;
  const int kProducers = 8;
  const int kPerProducer = 1000;
  const int kTotal = kProducers * kPerProducer;

  std::atomic<int> pushed{0};
  std::atomic<int> popped{0};
  std::vector<std::thread> producers;
  std::thread consumer([&]() {
    int val;
    int local_popped = 0;
    while (local_popped < kTotal) {
      if (q.Pop(val)) {
        ++local_popped;
        ++popped;
      }
    }
  });

  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p]() {
      int base = p * kPerProducer;
      for (int i = 0; i < kPerProducer; ++i) {
        q.Push(base + i);
        ++pushed;
      }
    });
  }

  for (auto& t : producers) {
    t.join();
  }
  consumer.join();

  EXPECT_EQ(pushed.load(), kTotal);
  EXPECT_EQ(popped.load(), kTotal);
  EXPECT_TRUE(q.Empty());
}
