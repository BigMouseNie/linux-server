#include "socket_wrap.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// SocketWrap implementation
SocketWrap::SocketWrap(SocketWrap&& other) : fd_(other.fd_) { other.fd_ = -1; }

SocketWrap& SocketWrap::operator=(SocketWrap&& other) {
  if (this == &other) return *this;
  Close();
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}

bool SocketWrap::IsNonBlock(bool* valid) const {
  if (!IsValid()) {
    if (valid) *valid = false;
    return false;
  }
  int flags = fcntl(fd_, F_GETFL, 0);
  if (valid) *valid = true;
  return (flags & O_NONBLOCK) != 0;
}

void SocketWrap::Close() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

// ServerSocket implementation
ServerSocket::ServerSocket(uint16_t port, bool ipv6, bool non_block,
                           int backlog) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int fd = socket(domain, SOCK_STREAM, 0);
  if (fd < 0) {
    return;
  }

  if (non_block) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = domain;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return;
  }

  if (listen(fd, backlog) < 0) {
    close(fd);
    return;
  }

  SetFd(fd);
}

ServerSocket::ServerSocket(const std::string& path, bool non_block) {
  unix_path_ = path;

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return;
  }

  if (non_block) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  unlink(path.c_str());

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return;
  }

  if (listen(fd, 128) < 0) {
    close(fd);
    return;
  }

  SetFd(fd);
}

ServerSocket::~ServerSocket() {
  if (!unix_path_.empty()) {
    unlink(unix_path_.c_str());
  }
}

int ServerSocket::Accept(struct sockaddr* client_addr, socklen_t* addr_len) {
  if (!IsValid()) {
    return -1;
  }

  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);

  int client_fd = accept(GetFd(), (struct sockaddr*)&addr, &len);
  if (client_fd < 0) {
    return -1;
  }

  if (client_addr && addr_len) {
    memcpy(client_addr, &addr, std::min(len, *addr_len));
    *addr_len = len;
  }

  return client_fd;
}

// ClientSocket implementation
ClientSocket::ClientSocket(bool ipv6, bool non_block) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int fd = socket(domain, SOCK_STREAM, 0);
  if (fd < 0) {
    return;
  }

  if (non_block) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  SetFd(fd);
}

ClientSocket::ClientSocket(const std::string& ip, uint16_t port, bool ipv6,
                           bool non_block) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int fd = socket(domain, SOCK_STREAM, 0);
  if (fd < 0) {
    return;
  }

  if (non_block) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    if (errno != EINPROGRESS) {
      close(fd);
      return;
    }
  }

  SetFd(fd);
  connected_ = true;
}

ClientSocket::ClientSocket(const std::string& path, bool non_block) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return;
  }

  if (non_block) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    if (errno != EINPROGRESS) {
      close(fd);
      return;
    }
  }

  SetFd(fd);
  connected_ = true;
}

ClientSocket::~ClientSocket() { connected_ = false; }

int ClientSocket::Connect(const std::string& ip, uint16_t port) {
  if (!IsValid()) {
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

  if (connect(GetFd(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    return -1;
  }

  connected_ = true;
  return 0;
}

int ClientSocket::Connect(const std::string& path) {
  if (!IsValid()) {
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(GetFd(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    return -1;
  }

  connected_ = true;
  return 0;
}
