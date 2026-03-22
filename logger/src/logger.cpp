#include "logger.h"

#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

Logger::~Logger() { Shutdown(); }

Logger& Logger::Instance() {
  static Logger kLogger;
  return kLogger;
}

int Logger::Init(const char* log_dir, LogLevel min_level, size_t max_entries) {
  if (running_.load()) return 0;

  log_dir_ = log_dir;
  min_level_ = min_level;
  max_entries_ = max_entries;
  cur_entries_ = 0;

  int ret = CreateLogFile();
  if (ret < 0) return -1;

  running_.store(true);
  writer_ = std::thread(&Logger::WriteLoop, this);
  return 0;
}

void Logger::Shutdown() {
  if (!running_.load()) return;
  running_.store(false);
  que_.Release();
  if (writer_.joinable()) {
    writer_.join();
  }
  if (p_log_file_) {
    fclose(p_log_file_);
    p_log_file_ = nullptr;
  }
}

void Logger::Log(LogLevel level, const char* file, int line, const char* fmt,
                 ...) {
  if (level < min_level_) return;
  if (!running_.load()) return;

  static const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  // 时间戳
  char timebuf[32];
  time_t now = time(nullptr);
  struct tm tm_time;
  localtime_r(&now, &tm_time);
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_time);

  // 格式化
  char msgbuf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
  va_end(args);

  // 提取文件名（去掉路径前缀）
  const char* basename = file;
  for (const char* p = file; *p; ++p) {
    if (*p == '/') basename = p + 1;
  }

  char linebuf[2048];
  int len =
      snprintf(linebuf, sizeof(linebuf), "[%s][%s][%s:%d] %s\n", timebuf,
               level_str[static_cast<int>(level)], basename, line, msgbuf);

  std::string log_line(linebuf, len > 0 ? len : 0);
  que_.Push(std::move(log_line));
}

void Logger::WriteLoop() {
  while (running_.load()) {
    std::string msg;
    if (que_.Pop(msg)) {
      RotateIfNeeded();
      if (p_log_file_) {
        fwrite(msg.c_str(), 1, msg.size(), p_log_file_);
        fflush(p_log_file_);
        ++cur_entries_;
      }
    } else {
      // 队列空且未被 release，短暂等待避免 busy loop
      if (running_.load()) {
        usleep(1000);
      }
    }
  }

  // drain remaining messages
  std::string msg;
  while (que_.Pop(msg)) {
    RotateIfNeeded();
    if (p_log_file_) {
      fwrite(msg.c_str(), 1, msg.size(), p_log_file_);
      fflush(p_log_file_);
      ++cur_entries_;
    }
  }
}

void Logger::RotateIfNeeded() {
  if (max_entries_ == 0 || cur_entries_ < max_entries_) return;
  cur_entries_ = 0;
  CreateLogFile();
}

int Logger::CreateLogFile() {
  if (CreateDirs(log_dir_.c_str()) < 0) return -1;

  char filename[64];
  char log_path[512];
  GenerateLogFileName(filename);
  snprintf(log_path, sizeof(log_path), "%s/%s", log_dir_.c_str(), filename);
  if (p_log_file_) {
    fclose(p_log_file_);
    p_log_file_ = nullptr;
  }
  p_log_file_ = fopen(log_path, "a");
  if (!p_log_file_) return -2;
  return 0;
}

void Logger::GenerateLogFileName(char* dest) {
  time_t now = time(nullptr);
  struct tm tm_time;
  localtime_r(&now, &tm_time);
  char timebuf[32];
  strftime(timebuf, sizeof(timebuf), "%Y%m%d_%H%M%S", &tm_time);
  sprintf(dest, "%s.%zu.log", timebuf, file_seq_++);
}

int Logger::CreateDirs(const char* path, mode_t mode) {
  if (path == nullptr || *path == '\0') return -1;

  std::string dir(path);
  size_t pos = 0;
  if (dir[0] == '/') pos = 1;
  while ((pos = dir.find('/', pos)) != std::string::npos) {
    std::string sub = dir.substr(0, pos++);
    if (sub.empty()) continue;
    if (access(sub.c_str(), F_OK) == 0) continue;
    if (mkdir(sub.c_str(), mode) != 0) {
      if (errno == EEXIST) continue;
      return -1;
    }
  }

  if (access(dir.c_str(), F_OK) != 0) {
    if (mkdir(dir.c_str(), mode) != 0 && errno != EEXIST) return -2;
  }

  return 0;
}
