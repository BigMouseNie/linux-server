#ifndef NETWORK_SOCKETWRAP_H_
#define NETWORK_SOCKETWRAP_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/socket.h>

class SocketWrap {
 public:
  explicit SocketWrap(int fd = -1) : fd_(fd) {}
  virtual ~SocketWrap() { Close(); }

  SocketWrap(SocketWrap&& other);
  SocketWrap& operator=(SocketWrap&& other);
  SocketWrap(const SocketWrap&) = delete;
  SocketWrap& operator=(const SocketWrap&) = delete;

  int GetFd() const { return fd_; }
  bool IsValid() const { return fd_ >= 0; }

  virtual void Close();

  int Release() {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }

 protected:
  void SetFd(int fd) { fd_ = fd; }

 private:
  int fd_ = -1;
};

class ServerSocket : public SocketWrap {
 public:
  ServerSocket(uint16_t port, bool ipv6 = false, bool non_block = true,
               int backlog = 128);
  explicit ServerSocket(const std::string& path, bool non_block = true);
  virtual ~ServerSocket() override;

  int Accept(struct sockaddr* client_addr = nullptr, socklen_t* addr_len = nullptr);

 private:
  std::string unix_path_;  // Unix socket
};

class ClientSocket : public SocketWrap {
 public:
  ClientSocket(bool ipv6 = false, bool non_block = true);
  ClientSocket(const std::string& ip, uint16_t port, bool ipv6 = false,
               bool non_block = true);
  explicit ClientSocket(const std::string& path, bool non_block = true);

  virtual ~ClientSocket() override;

  int Connect(const std::string& ip, uint16_t port);
  int Connect(const std::string& path);

 private:
  bool connected_ = false;
};

#endif  // NETWORK_SOCKETWRAP_H_
