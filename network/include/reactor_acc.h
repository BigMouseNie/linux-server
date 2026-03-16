#ifndef NETWORK_REACTORACC_H_
#define NETWORK_REACTORACC_H_

#include <memory>
#include <unordered_map>

#include "acceptor.h"
#include "epoller.h"
#include "socket_wrap.h"

class ReactorAcc : public Acceptor {
 public:
  ReactorAcc(AcceptCallBack cb, int listenfd_num, bool is_et = true);
  ~ReactorAcc() = default;

  bool IsValid() const { return epoller_ptr_ && epoller_ptr_->IsValid(); }
  int SetListenSock(ServerSocket&& sock);
  int Accept(int timeout_ms, int* saved_errno = nullptr);

 private:
  void Remove(int listnefd);

 private:
  std::unique_ptr<Epoller> epoller_ptr_;
  std::unordered_map<int, ServerSocket> listenfd_map_;
};

#endif  // NETWORK_REACTORACC_H_
