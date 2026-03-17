#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "thread_pool.h"

TEST(ThreadPoolTest, BasicTaskExecution) {
  ThreadPool pool(4);
  std::atomic<int> counter{0};

  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));
  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));
  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));

  // Wait for tasks to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter.load(), 3);
}

TEST(ThreadPoolTest, TaskWithManyArgs) {
  ThreadPool pool(2);
  std::atomic<int> result{0};

  auto add_func = [&result](int a, int b) { result = a + b; };

  EXPECT_TRUE(pool.AddTask(add_func, 10, 20));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(result.load(), 30);
}

TEST(ThreadPoolTest, ZeroThreadsBecomesOne) {
  // 0 threads becomes 1 by default
  ThreadPool pool(0);

  std::atomic<int> counter{0};
  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, NegativeThreadsBecomesOne) {
  ThreadPool pool(-5);

  std::atomic<int> counter{0};
  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, DestructorWaitsForTasks) {
  std::atomic<int> counter{0};

  {
    ThreadPool pool(4);

    for (int i = 0; i < 100; ++i) {
      pool.AddTask([&counter]() { counter++; });
    }

    // Destructor should wait for all tasks to complete
  }

  // All tasks should have completed
  EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, ConcurrentTasks) {
  ThreadPool pool(4);
  std::atomic<int> counter{0};
  const int num_tasks = 1000;

  for (int i = 0; i < num_tasks; ++i) {
    pool.AddTask([&counter]() {
      counter++;
      // Small delay to ensure concurrent execution
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    });
  }

  // Wait for all tasks
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_EQ(counter.load(), num_tasks);
}

TEST(ThreadPoolTest, DefaultThreadPoolSize) {
  // Default is 1 thread
  ThreadPool pool;

  std::atomic<int> counter{0};
  EXPECT_TRUE(pool.AddTask([&counter]() { counter++; }));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);
}