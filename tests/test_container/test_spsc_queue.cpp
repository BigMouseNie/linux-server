#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "spsc_queue.h"

// ---------- 基本功能测试 ----------

TEST(SPSCQueueTest, EmptyAfterCreate) {
  SPSCQueue<int> q(4);
  EXPECT_TRUE(q.Empty());
  EXPECT_EQ(q.Size(), 0);
}

TEST(SPSCQueueTest, PushAndPop) {
  SPSCQueue<int> q(4);
  EXPECT_TRUE(q.Push(1));
  EXPECT_TRUE(q.Push(2));
  EXPECT_EQ(q.Size(), 2);

  int val;
  EXPECT_TRUE(q.Pop(val));
  EXPECT_EQ(val, 1);
  EXPECT_TRUE(q.Pop(val));
  EXPECT_EQ(val, 2);
  EXPECT_EQ(q.Size(), 0);
  EXPECT_TRUE(q.Empty());
}

TEST(SPSCQueueTest, PopOnEmpty) {
  SPSCQueue<int> q(4);
  int val;
  EXPECT_FALSE(q.Pop(val));
}

TEST(SPSCQueueTest, PushOnFull) {
  // cap=4, RoundUpPow2(4)=4, 实际可存 3 个（留一个空位区分满/空）
  SPSCQueue<int> q(4);
  EXPECT_TRUE(q.Push(1));
  EXPECT_TRUE(q.Push(2));
  EXPECT_TRUE(q.Push(3));
  EXPECT_FALSE(q.Push(4));  // 满了
  EXPECT_EQ(q.Size(), 3);
}

TEST(SPSCQueueTest, MovePush) {
  SPSCQueue<std::string> q(4);
  std::string s = "hello";
  EXPECT_TRUE(q.Push(std::move(s)));
  // s 已被 move，可能为空
  std::string out;
  q.Pop(out);
  EXPECT_EQ(out, "hello");
}

TEST(SPSCQueueTest, WrapAround) {
  // 多轮 push/pop 验证环形缓冲区回绕
  SPSCQueue<int> q(8);
  for (int round = 0; round < 3; ++round) {
    for (int i = 0; i < 7; ++i) {
      EXPECT_TRUE(q.Push(i + round * 100));
    }
    for (int i = 0; i < 7; ++i) {
      int val;
      EXPECT_TRUE(q.Pop(val));
      EXPECT_EQ(val, i + round * 100);
    }
  }
  EXPECT_TRUE(q.Empty());
}

// ---------- 容量对齐测试 ----------

TEST(SPSCQueueTest, RoundUpPow2) {
  // 传入非 2 的幂，验证实际容量被对齐
  SPSCQueue<int> q(5);
  // RoundUpPow2(5) = 8, 实际可存 7 个
  for (int i = 0; i < 7; ++i) {
    EXPECT_TRUE(q.Push(i));
  }
  EXPECT_FALSE(q.Push(7));
}

TEST(SPSCQueueTest, MinCapacity) {
  SPSCQueue<int> q(1);
  // RoundUpPow2(1) = 2, 实际可存 1 个
  EXPECT_TRUE(q.Push(42));
  EXPECT_FALSE(q.Push(43));
}

// ---------- SPSC 并发测试 ----------

TEST(SPSCQueueTest, SingleProducerSingleConsumer) {
  SPSCQueue<int> q(1024);
  const int kCount = 100000;

  std::thread producer([&]() {
    for (int i = 0; i < kCount; ++i) {
      while (!q.Push(i)) {
        // busy wait until space available
      }
    }
  });

  int received = 0;
  std::thread consumer([&]() {
    int val;
    for (int i = 0; i < kCount; ++i) {
      while (!q.Pop(val)) {
        // busy wait until data available
      }
      EXPECT_EQ(val, i);
      ++received;
    }
  });

  producer.join();
  consumer.join();
  EXPECT_EQ(received, kCount);
  EXPECT_TRUE(q.Empty());
}
