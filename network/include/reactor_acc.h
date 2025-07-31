#ifndef NETWORK_REACTORACC_H_
#define NETWORK_REACTORACC_H_

#include <unordered_map>

#include "acceptor.h"
#include "epoller.h"
#include "socket_wrapper.h"

class ReactorAcc : public Acceptor {
 public:
  ReactorAcc() = default;
  ~ReactorAcc() = default;

  virtual int Create(AcceptCallBack cb, int listenfd_num, bool is_et);
  int SetListenSock(SocketWrapper&& sock);
  int Accept(int timeout_ms);

 private:
  void Remove(int listnefd);

 private:
  Epoller epoller_;
  std::unordered_map<int, SocketWrapper> listenfd_map_;
};

#endif  // NETWORK_REACTORACC_H_
