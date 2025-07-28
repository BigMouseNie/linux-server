#ifndef NETWORK_SOCKETWRAPPER_H_
#define NETWORK_SOCKETWRAPPER_H_

#include <stddef.h>
#include <stdint.h>

enum SocketAttr {
  kIsCreate = 0x01,
  kIsLocal = 0x02,
  kIsListen = 0x04,
  kIsIPV6 = 0x08,
  kIsUDP = 0x10,
  kIsNonBlock = 0x20,
};

class SocketWrapper {
 public:
  struct SocketCfg {
    uint16_t port;
    int sock_attr;
    int backlog;  // listen
    size_t address_size;
    char* address;  // ip or path
  };

 public:
  explicit SocketWrapper(bool manual_mgnt = true);
  SocketWrapper(const SocketWrapper&) = delete;
  SocketWrapper& operator=(const SocketWrapper&) = delete;
  SocketWrapper(SocketWrapper&& other);
  SocketWrapper& operator=(SocketWrapper&& other);
  ~SocketWrapper();

  int Create(const SocketCfg& sock_cfg);
  inline int GetSocket() { return sockfd_; }
  int Recv();
  int Send();
  void Close();

 private:
  int CreateSocket(const SocketCfg& sock_cfg);
  int CreateAddress(const SocketCfg& sock_cfg);
  int Bind(const SocketCfg& sock_cfg);
  // int Listen(const SocketCfg& sock_cfg);
  int SetNonBlock();

 private:
  int sock_attr_;
  int sockfd_;
  bool manual_mgnt_;
  struct sockaddr addr_;
};

#endif  // NETWORK_SOCKETWRAPPER_H_
