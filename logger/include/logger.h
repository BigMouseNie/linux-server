#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>

#include "blocking_queue.h"

enum class LogLevel { kDebug, kInfo, kWarn, kError, kFatal };

class Logger {
 public:
  static Logger& Instance();

  int Init(const char* log_dir, LogLevel min_level = LogLevel::kDebug,
           size_t max_entries = 10240);
  void Shutdown();

  void Log(LogLevel level, const char* file, int line, const char* fmt, ...);

 private:
  Logger() = default;
  ~Logger();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void WriteLoop();
  void RotateIfNeeded();
  int CreateDirs(const char* path, mode_t mode = 0755);
  void GenerateLogFileName(char* dest);
  int CreateLogFile();

 private:
  BlockingQueue<std::string> que_;
  std::thread writer_;
  FILE* p_log_file_ = nullptr;
  std::string log_dir_;
  LogLevel min_level_ = LogLevel::kDebug;
  size_t max_entries_ = 10240;  // 0 = 不限制
  size_t cur_entries_ = 0;
  size_t file_seq_ = 0;
  std::atomic<bool> running_{false};
};

#define LOG_DEBUG(fmt, ...)                                         \
  Logger::Instance().Log(LogLevel::kDebug, __FILE__, __LINE__, fmt, \
                         ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                         \
  Logger::Instance().Log(LogLevel::kInfo, __FILE__, __LINE__, fmt, \
                         ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                         \
  Logger::Instance().Log(LogLevel::kWarn, __FILE__, __LINE__, fmt, \
                         ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                         \
  Logger::Instance().Log(LogLevel::kError, __FILE__, __LINE__, fmt, \
                         ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)                                         \
  Logger::Instance().Log(LogLevel::kFatal, __FILE__, __LINE__, fmt, \
                         ##__VA_ARGS__)

#endif  // LOGGER_LOGGER_H_
