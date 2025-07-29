#ifndef NETWORK_SOCKETCREATOR_H_
#define NETWORK_SOCKETCREATOR_H_

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

using SockCfg = SocketCreator::SocketCfg;

class SocketCreator {
 public:
  struct SocketCfg {
    uint16_t port;
    int sock_attr;
    int backlog;  // listen
    size_t address_size;
    char* address;  // ip or path
  };

  static int Create(const SocketCfg& sock_cfg, int* attr = nullptr);

 private:
  SocketCreator() = default;
  ~SocketCreator() = default;
  void Close(int fd);
  int CreateSocket(const SocketCfg& sock_cfg);
  int CreateAddress(int fd, struct sockaddr_storage* addr,
                    const SocketCfg& sock_cfg);
  int Bind(int fd, struct sockaddr_storage* addr, const SocketCfg& sock_cfg);
  int Listen(int fd, const SocketCfg& sock_cfg);
  int SetNonBlock(int fd);
};

#endif  // NETWORK_SOCKETCREATOR_H_
