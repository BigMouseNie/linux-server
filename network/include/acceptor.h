#ifndef NETWORK_ACCEPTOR_H_
#define NETWORK_ACCEPTOR_H_

#include <functional>

class Acceptor {
 public:
  using AcceptCallBack =
      std::function<void(int conn_fd, struct sockaddr* addr, int addr_len)>;

  Acceptor(AcceptCallBack cb, bool is_et = true)
      : accept_cb_(std::move(cb)), is_et_(is_et) {}
  ~Acceptor() = default;

  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

  int DealConnFromSock(int sock);

 private:
  AcceptCallBack accept_cb_;
  bool is_et_;
};

#endif  // NETWORK_ACCEPTOR_H_
