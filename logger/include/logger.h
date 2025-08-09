#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <stdio.h>
#include <unistd.h>

#include <mutex>
#include <unordered_set>

#include "acceptor.h"
#include "epoller.h"
#include "socket_buffer.h"
#include "socket_wrapper.h"
#include "thread_wrapper.h"

enum class LogLevel { kDebug, kInfo, kWarn, kError, kFatal };

class Logger {
 public:
  static Logger& Instance();
  int Create();
  void Log(LogLevel level, const char* file, int line, const char* fmt, ...);

 private:
  Logger() : p_log_file_(nullptr), is_et_(true), runing_(false) {};
  ~Logger();

 private:
  int CreateLogFile();
  int CreateDirs(const char* path, mode_t mode = 0755);
  void GenerateLogFileName(char* dest);
  void Remove(int clientfd);
  void ReadSockToLogFile(int sock);
  int Worker();
  int Config();

 private:
  SocketCfg serv_sock_info_;
  SocketCfg clnt_sock_info_;
  SocketWrapper serv_sock_;
  Epoller epoller_;
  Acceptor acceptor_;
  ThreadWrapper worker_;
  std::unordered_set<int> clientfd_set_;
  std::mutex cfg_mtx_;
  SocketBuffer log_buf_;
  FILE* p_log_file_;
  char log_dir_[256];
  bool is_et_;
  bool runing_;
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
