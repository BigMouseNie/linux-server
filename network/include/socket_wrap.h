#ifndef NETWORK_SOCKETWRAP_H_
#define NETWORK_SOCKETWRAP_H_

#include <sys/socket.h>

#include <cstddef>
#include <cstdint>
#include <string>

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
  void SetManualMgmt(bool manual) { manual_mgmt_ = manual; }
  bool IsNonBlock(bool* valid = nullptr) const;

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
  bool manual_mgmt_ = false;
};

class ServerSocket : public SocketWrap {
 public:
  ServerSocket(uint16_t port, bool ipv6 = false, bool non_block = false,
               int backlog = 128);
  explicit ServerSocket(const std::string& path, bool non_block = false);
  virtual ~ServerSocket() override;

  int Accept(struct sockaddr* client_addr = nullptr,
             socklen_t* addr_len = nullptr);

 private:
  std::string unix_path_;  // Unix socket
};

class ClientSocket : public SocketWrap {
 public:
  ClientSocket(bool ipv6 = false, bool non_block = false);
  ClientSocket(const std::string& ip, uint16_t port, bool ipv6 = false,
               bool non_block = false);
  explicit ClientSocket(const std::string& path, bool non_block = false);

  virtual ~ClientSocket() override;

  int Connect(const std::string& ip, uint16_t port);
  int Connect(const std::string& path);
  bool IsConnected() const { return connected_; }

 private:
  bool connected_ = false;
};

#endif  // NETWORK_SOCKETWRAP_H_
