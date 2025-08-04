#ifndef NETWORK_SOCKETCREATOR_H_
#define NETWORK_SOCKETCREATOR_H_

#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

enum SocketAttr {
  kIsCreate = 0x01,
  kIsLocal = 0x02,
  kIsListen = 0x04,
  kIsIPV6 = 0x08,
  kIsUDP = 0x10,
  kIsNonBlock = 0x20,
};

class SocketCfg {
  friend class SocketCreator;

 public:
  SocketCfg()
      : port_(0),
        sock_attr_(0),
        backlog_(0),
        address_size_(0),
        address_(nullptr) {};
  ~SocketCfg() {
    if (address_) {
      delete[] address_;
      address_ = nullptr;
    }
  }

  SocketCfg(const SocketCfg& other);
  SocketCfg& operator=(const SocketCfg& other);
  SocketCfg(SocketCfg&& other);
  SocketCfg& operator=(SocketCfg&& other);

  void SetPort(uint16_t port) { port_ = port; }
  void SetSockAttr(int sock_attr) { sock_attr_ = sock_attr; };
  void SetBacklog(int backlog) { backlog_ = backlog; }
  int SetAddrOrPath(const char* addr_or_path);

  int GetPort() const { return port_; }
  int GetSockAttr() const { return sock_attr_; }
  int GetBacklog() const { return backlog_; }
  const char* GetAddress() const { return address_; }

 private:
  uint16_t port_;
  int sock_attr_;
  int backlog_;  // listen
  size_t address_size_;
  char* address_;  // ip or path
};

class SocketCreator {
 public:
  static SocketCreator& Instance();
  int Create(const SocketCfg& sock_cfg, int* attr = nullptr,
             struct sockaddr* addr = nullptr);

 private:
  SocketCreator() = default;
  ~SocketCreator() = default;
  void Close(int fd);
  int CreateSocket(const SocketCfg& sock_cfg);
  int CreateAddress(int fd, struct sockaddr* addr, const SocketCfg& sock_cfg);
  int Bind(int fd, struct sockaddr* addr, const SocketCfg& sock_cfg);
  int Listen(int fd, const SocketCfg& sock_cfg);
  int Connect(int fd, struct sockaddr* addr, const SocketCfg& sock_cfg);
  int SetNonBlock(int fd);

 private:
  struct sockaddr_storage t_addr_;
};

#endif  // NETWORK_SOCKETCREATOR_H_
