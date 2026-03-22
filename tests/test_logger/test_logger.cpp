#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "logger.h"

namespace fs = std::filesystem;

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = "/tmp/logger_test_" + std::to_string(getpid());
    fs::create_directories(test_dir_);
  }

  void TearDown() override {
    Logger::Instance().Shutdown();
    fs::remove_all(test_dir_);
  }

  int CountLogFiles() {
    int count = 0;
    for (const auto& entry : fs::directory_iterator(test_dir_)) {
      if (entry.is_regular_file() && entry.path().extension() == ".log") {
        ++count;
      }
    }
    return count;
  }

  std::string ReadAllLogs() {
    std::string result;
    for (const auto& entry : fs::directory_iterator(test_dir_)) {
      if (entry.is_regular_file() && entry.path().extension() == ".log") {
        std::ifstream f(entry.path());
        std::string line;
        while (std::getline(f, line)) {
          result += line + "\n";
        }
      }
    }
    return result;
  }

  std::string test_dir_;
};

TEST_F(LoggerTest, BasicInitLogShutdown) {
  Logger& logger = Logger::Instance();
  int ret = logger.Init(test_dir_.c_str(), LogLevel::kDebug);
  ASSERT_EQ(ret, 0);

  LOG_INFO("hello %s %d", "world", 42);

  logger.Shutdown();

  std::string logs = ReadAllLogs();
  EXPECT_NE(logs.find("[INFO]"), std::string::npos);
  EXPECT_NE(logs.find("hello world 42"), std::string::npos);
}

TEST_F(LoggerTest, LogLevelFilter) {
  Logger& logger = Logger::Instance();
  logger.Init(test_dir_.c_str(), LogLevel::kWarn);

  LOG_DEBUG("should not appear");
  LOG_INFO("should not appear either");
  LOG_WARN("warn should appear");
  LOG_ERROR("error should appear");
  LOG_FATAL("fatal should appear");

  logger.Shutdown();

  std::string logs = ReadAllLogs();
  EXPECT_EQ(logs.find("[DEBUG]"), std::string::npos);
  EXPECT_EQ(logs.find("[INFO]"), std::string::npos);
  EXPECT_NE(logs.find("[WARN]"), std::string::npos);
  EXPECT_NE(logs.find("warn should appear"), std::string::npos);
  EXPECT_NE(logs.find("[ERROR]"), std::string::npos);
  EXPECT_NE(logs.find("error should appear"), std::string::npos);
  EXPECT_NE(logs.find("[FATAL]"), std::string::npos);
  EXPECT_NE(logs.find("fatal should appear"), std::string::npos);
}

TEST_F(LoggerTest, MultiThreadLogging) {
  Logger& logger = Logger::Instance();
  logger.Init(test_dir_.c_str(), LogLevel::kInfo);

  const int kThreadCount = 8;
  const int kLogsPerThread = 100;
  std::vector<std::thread> threads;

  for (int t = 0; t < kThreadCount; ++t) {
    threads.emplace_back([t, kLogsPerThread]() {
      for (int i = 0; i < kLogsPerThread; ++i) {
        LOG_INFO("thread=%d iter=%d", t, i);
      }
    });
  }

  for (auto& th : threads) {
    th.join();
  }

  logger.Shutdown();

  std::string logs = ReadAllLogs();

  // 验证日志行数
  int line_count = 0;
  for (char c : logs) {
    if (c == '\n') ++line_count;
  }
  EXPECT_EQ(line_count, kThreadCount * kLogsPerThread);

  // 抽样验证某些行存在
  EXPECT_NE(logs.find("thread=0 iter=0"), std::string::npos);
  EXPECT_NE(logs.find("thread=3 iter=50"), std::string::npos);
  EXPECT_NE(logs.find("thread=7 iter=99"), std::string::npos);
}

TEST_F(LoggerTest, ShutdownDrainsRemainingLogs) {
  Logger& logger = Logger::Instance();
  logger.Init(test_dir_.c_str(), LogLevel::kDebug);

  // 快速写入大量日志后立即 Shutdown
  for (int i = 0; i < 1000; ++i) {
    LOG_INFO("drain test %d", i);
  }

  logger.Shutdown();

  std::string logs = ReadAllLogs();

  int line_count = 0;
  for (char c : logs) {
    if (c == '\n') ++line_count;
  }
  EXPECT_EQ(line_count, 1000);
  EXPECT_NE(logs.find("drain test 0"), std::string::npos);
  EXPECT_NE(logs.find("drain test 999"), std::string::npos);
}

TEST_F(LoggerTest, NoInitLogDoesNotCrash) {
  // 未 Init 就调用 Log，不应崩溃
  EXPECT_NO_THROW({ LOG_INFO("this should be silently ignored"); });
}

TEST_F(LoggerTest, DoubleInitIsHarmless) {
  Logger& logger = Logger::Instance();
  logger.Init(test_dir_.c_str(), LogLevel::kDebug);
  int ret = logger.Init(test_dir_.c_str(), LogLevel::kInfo);
  EXPECT_EQ(ret, 0);

  LOG_INFO("after double init");

  logger.Shutdown();

  std::string logs = ReadAllLogs();
  EXPECT_NE(logs.find("after double init"), std::string::npos);
}

TEST_F(LoggerTest, FileRotation) {
  Logger& logger = Logger::Instance();
  // 每个文件最多 3 条日志，写 10 条应该产生 ceil(10/3)=4 个文件
  const size_t kMaxEntries = 300;
  logger.Init(test_dir_.c_str(), LogLevel::kDebug, kMaxEntries);

  for (int i = 0; i < 1000; ++i) {
    LOG_INFO("rotation %d", i);
  }

  logger.Shutdown();

  int file_count = CountLogFiles();
  EXPECT_EQ(file_count, 4);

  std::string logs = ReadAllLogs();
  // 确保所有日志都在
  for (int i = 0; i < 10; ++i) {
    std::string expected = "rotation " + std::to_string(i);
    EXPECT_NE(logs.find(expected), std::string::npos)
        << "missing: " << expected;
  }
}
