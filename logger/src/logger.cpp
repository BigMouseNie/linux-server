#include "logger.h"

#include <stdarg.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

Logger::~Logger() {
  serv_sock_.SetManualMgnt(false);
  if (p_log_file_) {
    fclose(p_log_file_);
    p_log_file_ = nullptr;
  }
}

Logger& Logger::Instance() {
  static Logger kLogger;
  static bool kConfigured = false;
  if (!kConfigured) {
    std::lock_guard<std::mutex> lock(kLogger.cfg_mtx_);
    if (!kConfigured) {
      kLogger.Config();
      kConfigured = true;
    }
  }
  return kLogger;
}

int Logger::Create() {
  int ret = CreateLogFile();
  if (ret < 0) {
    return -1;
  }

  ret = serv_sock_.Create(serv_sock_info_);
  serv_sock_.SetManualMgnt(false);
  if (ret < 0) return -2;

  Acceptor::AcceptCallBack acc_cb = [this](int conn_fd, struct sockaddr* addr,
                                           int addr_len) {
    if (epoller_.Add(conn_fd, EPOLLIN) == 0) {
      clientfd_set_.insert(conn_fd);
    }
  };

  Epoller::EventsCallBack ev_cb = [this](epoll_event* evs, size_t size) {
    for (int i = 0; i < size; ++i) {
      int fd = evs[i].data.fd;
      // listen fd
      if (fd == serv_sock_.GetSocket()) {
        if (evs[i].events & EPOLLERR || (acceptor_.DealConnFromSock(fd) != 0)) {
          runing_ = false;
          return;
        }
        continue;
      }

      // client fd
      if (evs[i].events & EPOLLERR) Remove(fd);
      ReadSockToLogFile(fd);
    }
  };

  if (acceptor_.Create(acc_cb, is_et_) != 0) return -2;
  if (epoller_.Create(ev_cb, 32, is_et_) != 0) return -3;

  // run worker
  ret = worker_.Init(&Logger::Worker, this);
  if (ret < 0) return -4;
  runing_ = true;
  ret = worker_.Run();
  if (ret < 0) {
    return -5;
  }
  return 0;
}

int Logger::Worker() {
  uint32_t event = EPOLLIN;
  if (is_et_) event |= EPOLLET;
  int ret = epoller_.Add(serv_sock_.GetSocket(), event);
  if (ret != 0) return -1;

  while (runing_) {
    runing_ = (epoller_.Wait(500) >= 0);
  }
  runing_ = false;
  serv_sock_.Close();
  return 0;
}

int Logger::Config() {
  // 后续可以在install添加一个系统变量来控制
  const char* cfg_path =
      "/home/charstar/projects/playback-node/logger/config/logger_config.json";
  std::ifstream file(cfg_path);
  if (!file.is_open()) return -1;
  std::stringstream buffer;
  buffer << file.rdbuf();
  json j = json::parse(buffer.str(), nullptr, /* allow_exceptions = */ false);
  if (j.is_discarded()) return -2;

  std::string domain = j.value("domain", "LOCAL");
  std::string type = j.value("type", "STREAM");
  std::string address = j.value("address", "/home/charstar/tmp/log.sock");
  int port = j.value("port", 0);
  int backlog = j.value("backlog", 5);
  bool edge_trigger = j.value("edge_trigger", true);
  std::string log_dir = j.value("log_dir", "/home/charstar/log");

  int sock_attr = 0;
  if (domain == "LOCAL") sock_attr |= kIsLocal;
  if (domain == "INET6") sock_attr |= kIsIPV6;
  if (type == "DGRAM") sock_attr |= kIsUDP;
  if (edge_trigger == true) sock_attr |= kIsNonBlock;

  clnt_sock_info_.SetSockAttr(sock_attr);
  sock_attr |= kIsListen;
  serv_sock_info_.SetSockAttr(sock_attr);

  clnt_sock_info_.SetAddrOrPath(address.c_str());
  serv_sock_info_.SetAddrOrPath(address.c_str());

  clnt_sock_info_.SetPort(port);
  serv_sock_info_.SetPort(port);

  serv_sock_info_.SetBacklog(backlog);

  strcpy(log_dir_, log_dir.c_str());

  return 0;
}

int Logger::CreateLogFile() {
  if (CreateDirs(log_dir_) < 0) return -1;

  char filename[64];
  char log_path[320];
  GenerateLogFileName(filename);
  sprintf(log_path, "%s/%s", log_dir_, filename);
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
  pid_t pid = getpid();
  sprintf(dest, "%s.%d.log", timebuf, pid);
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

void Logger::Remove(int clientfd) {
  auto it = clientfd_set_.find(clientfd);
  if (it == clientfd_set_.end()) return;
  close(*it);
  epoller_.Del(clientfd);
  clientfd_set_.erase(it);
}

void Logger::ReadSockToLogFile(int sock) {
  int saved_errno = 0;
  int read_len = log_buf_.ReadFromSock(sock, is_et_, &saved_errno);
  if (read_len <= 0) {
    Remove(sock);
    return;
  }

  // write log file
  fwrite(log_buf_.GetReadPtr(), sizeof(char), log_buf_.Readable(), p_log_file_);
  log_buf_.Clear();
  fflush(p_log_file_);
}

void Logger::Log(LogLevel level, const char* file, int line, const char* fmt,
                 ...) {
  static thread_local SocketWrapper clnt_sock;
  static thread_local SocketBuffer buffer;
  static const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  if (!clnt_sock.IsValid()) {
    buffer.Resize(2048);
    int ret = clnt_sock.Create(clnt_sock_info_);
    if (ret < 0) return;
  }

  // 时间戳
  char timebuf[32];
  time_t now = time(nullptr);
  struct tm tm_time;
  localtime_r(&now, &tm_time);
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_time);

  // 格式化用户日志内容
  char msgbuf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
  va_end(args);

  int len = sprintf(buffer.GetWritePtr(), "[%s][%s][%s:%d] %s\n", timebuf,
                    level_str[(int)level], file, line, msgbuf);
  buffer.Written(len);
  int ret = clnt_sock.Send(buffer);
  buffer.Clear();
}
