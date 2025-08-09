#ifndef NETWORK_SOCKETWRAPPER_H_
#define NETWORK_SOCKETWRAPPER_H_

#include <stddef.h>
#include <stdint.h>

#include "socket_creator.h"

class SocketBuffer;
class SocketWrapper {
 public:
  explicit SocketWrapper(bool manual_mgnt = true);
  ~SocketWrapper() {
    if (!manual_mgnt_) Close();
  }

  SocketWrapper(SocketWrapper&& other);
  SocketWrapper& operator=(SocketWrapper&& other);

  int Create(const SocketCfg& sock_cfg, struct sockaddr* addr = nullptr);
  void Close();
  int GetSocket() { return sockfd_; }
  void SetManualMgnt(bool manual) { manual_mgnt_ = manual; }
  bool IsValid() { return attr_ & kIsCreate; }
  bool IsNonBlock() { return attr_ & kIsNonBlock; }

  int Recv(SocketBuffer& buf);
  int Send(SocketBuffer& buf);

 private:
  int attr_;
  int sockfd_;
  bool manual_mgnt_;
};

#endif  // NETWORK_SOCKETWRAPPER_H_
