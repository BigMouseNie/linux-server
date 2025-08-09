#ifndef NETWORK_ACCEPTOR_H_
#define NETWORK_ACCEPTOR_H_

#include <functional>

class Acceptor {
 public:
  using AcceptCallBack = std::function<void(int conn_fd, struct sockaddr* addr,
                                            int addr_len)>;
  Acceptor() : is_et_(true) {}
  ~Acceptor() = default;
  virtual int Create(AcceptCallBack cb, bool is_et);
  int DealConnFromSock(int sock);

 protected:
  AcceptCallBack accept_cb_;
  bool is_et_;
};

#endif  // NETWORK_ACCEPTOR_H_
