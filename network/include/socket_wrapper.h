#ifndef NETWORK_SOCKETWRAPPER_H_
#define NETWORK_SOCKETWRAPPER_H_

#include <stddef.h>
#include <stdint.h>

#include "socket_buffer.h"
#include "socket_creator.h"

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
  ~SocketWrapper() {
    if (!manual_mgnt_) Close();
  }

  SocketWrapper(const SocketWrapper&) = delete;
  SocketWrapper& operator=(const SocketWrapper&) = delete;

  SocketWrapper(SocketWrapper&& other);
  SocketWrapper& operator=(SocketWrapper&& other);

  int Create(const SockCfg& sock_cfg);
  void Close();
  int GetSocket() { return sockfd_; }
  void SetManualMgnt(bool manual) { manual_mgnt_ = manual; }
  bool isValid() { return attr_ & kIsCreate; }

  int Recv(SocketBuffer& buf);
  int Send(SocketBuffer& buf);

 private:
  int attr_;
  int sockfd_;
  bool manual_mgnt_;
};

#endif  // NETWORK_SOCKETWRAPPER_H_
